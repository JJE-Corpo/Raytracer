//
// Created by jazema on 4/21/26.
//

#include "Scene.hpp"
#include "../../common/Intersection.hpp"
#include "../../common/Ray.hpp"
#include "../../common/AABB.hpp"
#include "../../common/scene/IEditablePrimitive.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <filesystem>

#include "factory/PrimitiveFactory.hpp"
#include "factory/LightFactory.hpp"
#include "../../plugins/primitive/mesh/Mesh.hpp"
#include "../../plugins/primitive/Cube.hpp"

namespace rc
{
    namespace
    {
        // Component-wise vector product / quotient used by the transform compose.
        Vector3f compMul(const Vector3f &a, const Vector3f &b)
        {
            return {a.x * b.x, a.y * b.y, a.z * b.z};
        }

        Vector3f compDiv(const Vector3f &a, const Vector3f &b)
        {
            auto safe = [](float d) { return (std::fabs(d) < 1e-6f) ? (d < 0.0f ? -1e-6f : 1e-6f) : d; };
            return {a.x / safe(b.x), a.y / safe(b.y), a.z / safe(b.z)};
        }
    }

    Scene::Scene(ICamera *camera, float ambientCoefficient, float diffuseCoefficient): _camera(camera), _ambientCoefficient(ambientCoefficient), _diffuseCoefficient(diffuseCoefficient)
    {
    }

    Scene::~Scene()
    {
        this->Scene::clear();
        this->_mutex.lock();
        delete this->_camera;
        this->_mutex.unlock();
    }

    void Scene::addRoot(ISceneObject *object)
    {
        if (!object)
            return;
        if (std::find(this->_roots.begin(), this->_roots.end(), object) == this->_roots.end())
            this->_roots.push_back(object);
    }

    void Scene::insertRoot(ISceneObject *object, int index)
    {
        if (!object)
            return;
        if (std::find(this->_roots.begin(), this->_roots.end(), object) != this->_roots.end())
            return;
        if (index < 0 || index > static_cast<int>(this->_roots.size()))
            this->_roots.push_back(object);
        else
            this->_roots.insert(this->_roots.begin() + index, object);
    }

    void Scene::removeRoot(ISceneObject *object)
    {
        auto it = std::find(this->_roots.begin(), this->_roots.end(), object);
        if (it != this->_roots.end())
            this->_roots.erase(it);
    }

    void Scene::addPrimitive(IPrimitive *primitive)
    {
        this->_mutex.lock();
        this->_primitives.push_back(primitive);
        // Nested objects (already parented by the parser/reparent) are not roots.
        if (primitive && primitive->getParent() == nullptr)
            this->addRoot(primitive);
        this->_mutex.unlock();
    }

    void Scene::addLight(ILight *light)
    {
        this->_mutex.lock();
        this->_lights.push_back(light);
        if (light && light->getParent() == nullptr)
            this->addRoot(light);
        this->_mutex.unlock();
    }

    void Scene::adoptGroup(Group *group)
    {
        if (!group)
            return;
        this->_mutex.lock();
        this->_groups.push_back(group);
        if (group->getParent() == nullptr)
            this->addRoot(group);
        this->_mutex.unlock();
    }

    ISceneObject *Scene::addGroup()
    {
        Group *group = new Group();

        this->adoptGroup(group);
        return group;
    }

    void Scene::reparent(ISceneObject *child, ISceneObject *newParent, int index)
    {
        if (!child || child == newParent)
            return;
        // Only groups may hold children.
        if (newParent && newParent->getObjectType() != ObjectType::GROUP)
            return;
        // Reject cycles: newParent must not be child itself or a descendant of it.
        for (ISceneObject *p = newParent; p != nullptr; p = p->getParent())
            if (p == child)
                return;

        ISceneObject *oldParent = child->getParent();
        // A same-parent move only reorders siblings: world and parent are
        // unchanged, so the local transform must be left exactly as-is (a
        // recompute would only inject floating-point drift).
        const bool sameParent = (oldParent == newParent);

        // Snapshot the current world transform so the object stays put visually
        // across an actual reparent.
        const Vector3f wPos = child->getPosition();
        const Vector3f wRot = child->getRotation();
        const Vector3f wScale = child->getScale();

        this->_mutex.lock();
        if (oldParent)
            oldParent->removeChild(child);
        else
            this->removeRoot(child);

        if (newParent)
        {
            if (index < 0)
                newParent->addChild(child); // append; sets child's parent
            else
                newParent->insertChild(child, static_cast<std::size_t>(index));

            if (!sameParent)
            {
                const Vector3f pPos = newParent->getPosition();
                const Vector3f pRot = newParent->getRotation();
                const Vector3f pScale = newParent->getScale();

                // Inverse of the flatten compose, so world stays fixed.
                child->setLocalScale(compDiv(wScale, pScale));
                child->setLocalRotation(wRot - pRot);
                child->setLocalPosition(compDiv(inverseRotate(wPos - pPos, degToRad(pRot)), pScale));
            }
        }
        else
        {
            child->setParent(nullptr);
            this->insertRoot(child, index);
        }
        this->_mutex.unlock();
    }

    bool Scene::isOwned(const ISceneObject *object) const
    {
        if (!object)
            return (false);
        // Up-cast each owning pointer to the shared base to compare identity;
        // ISceneObject is a virtual base, so a downcast of `object` is not
        // available here and would be undefined for an already-freed pointer.
        for (const IPrimitive *primitive : this->_primitives)
            if (static_cast<const ISceneObject *>(primitive) == object)
                return (true);
        for (const ILight *light : this->_lights)
            if (static_cast<const ISceneObject *>(light) == object)
                return (true);
        for (const Group *group : this->_groups)
            if (static_cast<const ISceneObject *>(group) == object)
                return (true);
        return (false);
    }

    void Scene::destroyObjectSubtree(ISceneObject *object)
    {
        if (!object)
            return;
        // Snapshot the children before recursing: the recursive delete frees
        // them but leaves this node's own child vector untouched, so a copy is
        // safe either way.
        const std::vector<ISceneObject *> children = object->getChildren();
        for (ISceneObject *child : children)
            this->destroyObjectSubtree(child);

        switch (object->getObjectType())
        {
            case ObjectType::PRIMITIVE:
                for (auto it = this->_primitives.begin(); it != this->_primitives.end(); ++it)
                    if (static_cast<ISceneObject *>(*it) == object) { this->_primitives.erase(it); break; }
                // Infinite primitives (e.g. planes) also live in this cache until
                // the next buildBvh(); drop the pointer now so nothing dangles.
                for (auto it = this->_infinitePrimitives.begin(); it != this->_infinitePrimitives.end(); ++it)
                    if (static_cast<ISceneObject *>(*it) == object) { this->_infinitePrimitives.erase(it); break; }
                break;
            case ObjectType::LIGHT:
                for (auto it = this->_lights.begin(); it != this->_lights.end(); ++it)
                    if (static_cast<ISceneObject *>(*it) == object) { this->_lights.erase(it); break; }
                break;
            case ObjectType::GROUP:
                for (auto it = this->_groups.begin(); it != this->_groups.end(); ++it)
                    if (static_cast<ISceneObject *>(*it) == object) { this->_groups.erase(it); break; }
                break;
        }
        delete object;
    }

    void Scene::removeObject(ISceneObject *object)
    {
        if (!object)
            return;
        this->_mutex.lock();
        // Guard against a double removal (e.g. a group and one of its
        // descendants both queued for deletion): once the group's subtree is
        // freed the descendant is no longer owned, so skip it rather than touch
        // freed memory.
        if (this->isOwned(object))
        {
            ISceneObject *parent = object->getParent();
            if (parent)
                parent->removeChild(object);
            else
                this->removeRoot(object);
            this->destroyObjectSubtree(object);
        }
        this->_mutex.unlock();
    }

    namespace
    {
        struct TessMesh
        {
            std::vector<Vector3f> verts;
            std::vector<Vector3f> normals;
            std::vector<std::array<int, 3>> faces;
        };

        // Generic surface tessellation: cast rays from a UV sphere of directions
        // toward the primitive's centre and keep the first hit. Produces standard
        // UV-sphere topology (two poles + rings). Works for any star-shaped
        // primitive w.r.t. its centre (cube, sphere, cone, cylinder); non-convex
        // shapes (torus, tanglecube) are approximated. Per-vertex normals come
        // from the surface hit, so the mesh shades smoothly.
        bool tessellatePrimitive(const IPrimitive *prim, int stacks, int slices, TessMesh &out)
        {
            AABB box = prim->bounding_box();
            Vector3f center = (box.min + box.max) * 0.5f;
            Vector3f ext = box.max - box.min;
            float radius = std::sqrt(ext.x * ext.x + ext.y * ext.y + ext.z * ext.z) * 0.5f + 1e-2f;
            if (radius <= 0.0f)
                return (false);

            auto sample = [&](const Vector3f &dir, Vector3f &p, Vector3f &n)
            {
                Ray ray(center + dir * (radius * 2.0f), dir * -1.0f);
                Intersection hit;
                if (prim->intersect(ray, 1e-3f, radius * 4.0f, hit))
                {
                    p = hit.point;
                    n = hit.normal;
                }
                else
                {
                    p = center + dir * radius; // fallback: bounding-sphere point
                    n = dir;
                }
            };

            Vector3f p, n;
            sample(Vector3f(0.0f, 0.0f, 1.0f), p, n);
            out.verts.push_back(p); out.normals.push_back(n);
            for (int i = 1; i < stacks; ++i)
            {
                const float theta = static_cast<float>(M_PI) * i / stacks;
                const float st = std::sin(theta), ct = std::cos(theta);
                for (int j = 0; j < slices; ++j)
                {
                    const float phi = 2.0f * static_cast<float>(M_PI) * j / slices;
                    Vector3f dir(st * std::cos(phi), st * std::sin(phi), ct);
                    sample(dir, p, n);
                    out.verts.push_back(p); out.normals.push_back(n);
                }
            }
            sample(Vector3f(0.0f, 0.0f, -1.0f), p, n);
            out.verts.push_back(p); out.normals.push_back(n);
            const int south = static_cast<int>(out.verts.size()) - 1;

            auto ring = [&](int i, int j) { return 1 + (i - 1) * slices + (j % slices); };
            for (int j = 0; j < slices; ++j)
                out.faces.push_back({0, ring(1, j), ring(1, j + 1)});
            for (int i = 1; i < stacks - 1; ++i)
                for (int j = 0; j < slices; ++j)
                {
                    out.faces.push_back({ring(i, j), ring(i, j + 1), ring(i + 1, j + 1)});
                    out.faces.push_back({ring(i, j), ring(i + 1, j + 1), ring(i + 1, j)});
                }
            for (int j = 0; j < slices; ++j)
                out.faces.push_back({south, ring(stacks - 1, j + 1), ring(stacks - 1, j)});
            return (!out.faces.empty());
        }

        // Exact 8-vertex / 12-triangle tessellation of a cube (flat faces), so a
        // converted cube stays a proper box you can edit corner by corner instead
        // of a dense generic sampling.
        bool tessellateCube(const Cube *cube, TessMesh &out)
        {
            Vector3f corners[8];
            cube->getWorldCorners(corners);
            out.verts.assign(corners, corners + 8);
            out.normals.clear(); // flat shading -> hard cube edges
            static const int FACE[6][4] = {
                {0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 6, 5},
                {1, 2, 7, 6}, {2, 3, 4, 7}, {0, 3, 4, 5},
            };
            for (int f = 0; f < 6; ++f)
            {
                out.faces.push_back({FACE[f][0], FACE[f][1], FACE[f][2]});
                out.faces.push_back({FACE[f][0], FACE[f][2], FACE[f][3]});
            }
            return (true);
        }

        std::string sanitizeName(std::string name)
        {
            for (char &c : name)
                if (!std::isalnum(static_cast<unsigned char>(c)))
                    c = '_';
            if (name.empty())
                name = "mesh";
            return (name);
        }

        bool writeObjFile(const std::string &path, const TessMesh &m)
        {
            std::ofstream file(path);
            if (!file.is_open())
                return (false);
            file << "# Generated by convert-to-mesh\n";
            for (const Vector3f &v : m.verts)
                file << "v " << v.x << " " << v.y << " " << v.z << "\n";
            for (const Vector3f &n : m.normals)
                file << "vn " << n.x << " " << n.y << " " << n.z << "\n";
            // With per-vertex normals the faces reference them (smooth shading);
            // without, plain "f a b c" leaves the faces flat (hard edges, e.g. a cube).
            const bool smooth = !m.normals.empty();
            for (const std::array<int, 3> &t : m.faces)
            {
                if (smooth)
                    file << "f " << t[0] + 1 << "//" << t[0] + 1 << " "
                         << t[1] + 1 << "//" << t[1] + 1 << " "
                         << t[2] + 1 << "//" << t[2] + 1 << "\n";
                else
                    file << "f " << t[0] + 1 << " " << t[1] + 1 << " " << t[2] + 1 << "\n";
            }
            return (true);
        }
    }

    IPrimitive *Scene::convertToMesh(IPrimitive *primitive)
    {
        if (!primitive || !primitive->isFinite())
            return (nullptr);
        // Already a vertex-editable primitive (mesh / triangle): nothing to do.
        if (dynamic_cast<IEditablePrimitive *>(primitive))
            return (nullptr);

        TessMesh tess;
        // A cube has an exact 8-corner form; everything else is sampled generically.
        if (const Cube *cube = dynamic_cast<const Cube *>(primitive))
        {
            if (!tessellateCube(cube, tess))
                return (nullptr);
        }
        else if (!tessellatePrimitive(primitive, 32, 48, tess))
        {
            return (nullptr);
        }

        Vector3f mn = tess.verts[0], mx = tess.verts[0];
        for (const Vector3f &v : tess.verts)
        {
            mn.x = std::min(mn.x, v.x); mn.y = std::min(mn.y, v.y); mn.z = std::min(mn.z, v.z);
            mx.x = std::max(mx.x, v.x); mx.y = std::max(mx.y, v.y); mx.z = std::max(mx.z, v.z);
        }
        const Vector3f center = (mn + mx) * 0.5f;

        std::error_code ec;
        std::filesystem::create_directories("generated_meshes", ec);
        const std::string path = "generated_meshes/" + sanitizeName(primitive->getName())
            + "_" + std::to_string(++this->_meshExportCounter) + ".obj";
        if (!writeObjFile(path, tess))
            return (nullptr);

        // Read what we need before removeObject() frees the primitive.
        const Material *material = primitive->getMaterial();
        const std::string name = primitive->getName();

        Mesh *mesh = new Mesh(name, path, center, Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f, 1.0f, 1.0f), material);
        this->addPrimitive(mesh);
        this->removeObject(primitive);
        // Rebuild immediately so the freed primitive can never be reached through
        // a stale BVH. The caller still refreshes the viewport.
        this->buildBvh();
        return (mesh);
    }

    void Scene::addDefaultPrimitive(std::string type)
    {
        IPrimitive *primitive = PrimitiveFactory::createPrimitive(type);
        if (primitive)
            addPrimitive(primitive);
    }

    void Scene::addDefaultLight(std::string type)
    {
        ILight *light = LightFactory::createLight(type);
        if (light)
            addLight(light);
    }

    Material *Scene::getMaterial(const std::string &name)
    {
        if (this->_materials.find(name) != this->_materials.end())
            return (&this->_materials[name]);
        return (nullptr);
    }

    Material *Scene::createMaterial(const std::string &name)
    {
        if (this->_materials.find(name) != this->_materials.end())
            return (&this->_materials[name]);
        Material result;

        result.name = name;
        this->_materials.emplace(name, result);
        return (&this->_materials[name]);
    }

    Material *Scene::createMaterial()
    {
        std::string name = "material #0";
        int tmp = 0;

        while (this->_materials.find(name) != this->_materials.end())
        {
            tmp++;
            name = "material #" + std::to_string(tmp);
        }
        return (createMaterial(name));
    }

    void Scene::clear()
    {
        this->_mutex.lock();
        while (!this->_primitives.empty())
        {
            delete this->_primitives.back();
            this->_primitives.pop_back();
        }
        while (!this->_lights.empty())
        {
            delete this->_lights.back();
            this->_lights.pop_back();
        }
        while (!this->_groups.empty())
        {
            delete this->_groups.back();
            this->_groups.pop_back();
        }
        this->_roots.clear();
        this->_materials.clear();
        this->_mutex.unlock();
    }

    float Scene::getAmbientCoefficient() const
    {
        return (this->_ambientCoefficient);
    }

    float Scene::getDiffuseCoefficient() const
    {
        return (this->_diffuseCoefficient);
    }

    void Scene::setAmbientCoefficient(float ambientCoefficient)
    {
        this->_mutex.lock();
        this->_ambientCoefficient = ambientCoefficient;
        this->_mutex.unlock();
    }

    void Scene::setDiffuseCoefficient(float diffuseCoefficient)
    {
        this->_mutex.lock();
        this->_diffuseCoefficient = diffuseCoefficient;
        this->_mutex.unlock();
    }

    void Scene::flattenNode(ISceneObject *object, const Vector3f &parentPos,
        const Vector3f &parentRot, const Vector3f &parentScale)
    {
        Vector3f worldPos;
        Vector3f worldRot;
        Vector3f worldScale;

        if (object->getParent() == nullptr)
        {
            // Roots keep their existing world transform untouched, so un-grouped
            // scenes are unchanged (the pass is a no-op for them).
            worldPos = object->getPosition();
            worldRot = object->getRotation();
            worldScale = object->getScale();
        }
        else
        {
            const Vector3f localPos = object->getLocalPosition();
            const Vector3f localRot = object->getLocalRotation();
            const Vector3f localScale = object->getLocalScale();

            worldScale = compMul(parentScale, localScale);
            worldRot = parentRot + localRot;
            worldPos = parentPos + rotate(compMul(parentScale, localPos), degToRad(parentRot));
            object->setPosition(worldPos);
            object->setRotation(worldRot);
            object->setScale(worldScale);
        }
        for (auto *child : object->getChildren())
            if (child)
                this->flattenNode(child, worldPos, worldRot, worldScale);
    }

    void Scene::flattenGraph()
    {
        for (auto *root : this->_roots)
            if (root)
                this->flattenNode(root, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
    }

    void Scene::buildBvh()
    {
        this->_mutex.lock();
        std::vector<BVHBuildItem> finitePrimitives;

        // Resolve world transforms from the graph before reading bounding boxes.
        this->flattenGraph();
        this->_infinitePrimitives.clear();
        for (auto *primitive : this->_primitives)
        {
            if (!primitive)
                continue;
            if (primitive->isHidden())
                continue;
            if (primitive->isFinite())
            {
                const AABB bounds = primitive->bounding_box();
                finitePrimitives.push_back({primitive, bounds, (bounds.min + bounds.max) * 0.5f});
            }
            else
                this->_infinitePrimitives.push_back(primitive);
        }
        this->_bvhRoot = std::make_unique<BVHNode>(finitePrimitives, 0, finitePrimitives.size());
        this->_mutex.unlock();
    }

    ICamera &Scene::getCamera() const
    {
        return (*this->_camera);
    }

    const std::vector<IPrimitive *> &Scene::getPrimitives() const
    {
        return (this->_primitives);
    }

    const std::vector<ILight *> &Scene::getLights() const
    {
        return (this->_lights);
    }

    const std::map<std::string, Material> &Scene::getMaterials() const
    {
        return (this->_materials);
    }

    const std::vector<ISceneObject *> &Scene::getRoots() const
    {
        return (this->_roots);
    }

    bool Scene::intersect(const Ray &ray, float tMin, float tMax, Intersection &hit) const
    {
        bool hitAnything = false;
        float closest = tMax;

        if (this->_bvhRoot)
        {
            Intersection temp;
            if (this->_bvhRoot->intersect(ray, tMin, closest, temp))
            {
                hitAnything = true;
                closest = temp.t;
                hit = temp;
            }
        }
        for (auto* obj : this->_infinitePrimitives)
        {
            if (!obj)
                continue;
            if (obj->isHidden())
                continue;
            Intersection temp;
            if (obj->intersect(ray, tMin, closest, temp))
            {
                hitAnything = true;
                closest = temp.t;
                hit = temp;
            }
        }
        return (hitAnything);
    }
}
