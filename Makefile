NAME = raytracer

SOURCES = main.cpp \
		srcs/core/Core.cpp \
		srcs/core/PluginLoader.cpp \
		srcs/common/scene/ASceneObject.cpp \
		srcs/core/scene/Scene.cpp \
		srcs/core/scene/SceneParser.cpp \
		srcs/core/scene/SceneRegister.cpp \
		srcs/core/scene/camera/Camera.cpp \
		srcs/core/scene/bvh/BVHNode.cpp \
		srcs/core/scene/builder/SceneObjectBuilder.cpp \
		srcs/core/scene/builder/LightBuilder.cpp \
		srcs/core/scene/builder/SceneBuilder.cpp \
		srcs/core/scene/builder/PrimitiveBuilder.cpp \
		srcs/core/cluster/ClusterModule.cpp \
		srcs/core/cluster/render/ClusterRenderCoordinator.cpp \
		srcs/core/cluster/render/ClusterRenderer.cpp \
		srcs/core/cluster/client/ClusterClient.cpp \
		srcs/core/cluster/server/ClusterServer.cpp \
		srcs/core/cluster/server/Connection.cpp \
		srcs/core/cluster/packets/PacketFactory.cpp \
		srcs/core/cluster/packets/PacketClientJoinRequest.cpp \
		srcs/core/cluster/packets/PacketClientFetchSceneData.cpp \
		srcs/core/cluster/packets/PacketClientTileData.cpp \
		srcs/core/cluster/packets/PacketServerJoinResponse.cpp \
		srcs/core/cluster/packets/PacketServerClusterData.cpp \
		srcs/core/cluster/packets/PacketServerSceneData.cpp \
		srcs/core/cluster/packets/PacketServerCancelRender.cpp \
		srcs/core/cluster/packets/PacketServerRenderRequest.cpp \
		srcs/core/cluster/packets/PacketServerRenderState.cpp \
		srcs/core/utils/RenderExporter.cpp \
		srcs/core/utils/FileObserver.cpp \
		srcs/core/exceptions/LoadingSceneException.cpp \
		srcs/core/exceptions/BuildingObjectException.cpp \
		srcs/core/exceptions/ExportRenderException.cpp \
		srcs/core/exceptions/ColorException.cpp \
		srcs/core/exceptions/VectorException.cpp \
		srcs/core/obj/ObjParser.cpp \
		srcs/plugins/light/PointLight.cpp \
		srcs/plugins/light/DirectionalLight.cpp \
		srcs/plugins/primitive/Sphere.cpp \
		srcs/plugins/primitive/Plane.cpp \
		srcs/plugins/primitive/Cylinder.cpp \
		srcs/plugins/primitive/Triangle.cpp \
		srcs/plugins/primitive/Tanglecube.cpp \
		srcs/plugins/primitive/Cube.cpp \
		srcs/plugins/primitive/Cone.cpp \
		srcs/plugins/primitive/Torus.cpp \
		srcs/plugins/primitive/Fractal.cpp \
		srcs/plugins/primitive/mesh/MeshTriangle.cpp \
		srcs/plugins/primitive/mesh/Mesh.cpp \
		srcs/common/render/stb_image_impl.cpp \

OBJECTS = ${SOURCES:.cpp=.o}

PLUGINS_PATH = ./plugins

USERINTERFACE_NAME = user_interface.so
USERINTERFACE_SOURCES = srcs/plugins/user_interface/UserInterface.cpp \
		srcs/plugins/user_interface/CursorManager.cpp \
		srcs/plugins/user_interface/panels/CameraPanel.cpp \
		srcs/plugins/user_interface/panels/HierarchyPanel.cpp \
		srcs/plugins/user_interface/panels/MaterialPanel.cpp \
		srcs/plugins/user_interface/panels/ObjectPanel.cpp \
		srcs/plugins/user_interface/panels/RendererPanel.cpp \
		srcs/plugins/user_interface/panels/Section.cpp \
		srcs/plugins/user_interface/panels/SidebarStack.cpp \
		srcs/plugins/user_interface/screens/DefaultScreen.cpp
USERINTERFACE_OBJECTS = ${USERINTERFACE_SOURCES:.cpp=.o}

DEFAULTRENDERER_NAME = renderer_default.so
DEFAULTRENDERER_SOURCES = srcs/plugins/renderer/DefaultRenderer.cpp \
		srcs/common/render/stb_image_impl.cpp
DEFAULTRENDERER_OBJECTS = ${DEFAULTRENDERER_SOURCES:.cpp=.o}

VIEWPORTRENDERER_NAME = renderer_viewport.so
VIEWPORTRENDERER_SOURCES = srcs/plugins/renderer/ViewportRenderer.cpp \
		srcs/common/render/stb_image_impl.cpp
VIEWPORTRENDERER_OBJECTS = ${VIEWPORTRENDERER_SOURCES:.cpp=.o}

all: $(NAME)

$(NAME): $(OBJECTS) $(USERINTERFACE_OBJECTS) $(DEFAULTRENDERER_OBJECTS) $(VIEWPORTRENDERER_OBJECTS)
		mkdir -p $(PLUGINS_PATH)
		clang++ -Wall -Wextra -Werror $(OBJECTS) -o $(NAME) -std=c++17
		clang++ -Wall -Wextra -Werror $(USERINTERFACE_OBJECTS) -o $(PLUGINS_PATH)/$(USERINTERFACE_NAME) -std=c++17 -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lX11 -fPIC -shared
		clang++ -Wall -Wextra -Werror $(DEFAULTRENDERER_OBJECTS) -o $(PLUGINS_PATH)/$(DEFAULTRENDERER_NAME) -std=c++17 -fPIC -shared
		clang++ -Wall -Wextra -Werror $(VIEWPORTRENDERER_OBJECTS) -o $(PLUGINS_PATH)/$(VIEWPORTRENDERER_NAME) -std=c++17 -fPIC -shared

%.o: %.cpp
		clang++ -Wall -Wextra -Werror -isystem srcs/external $< -o ${<:.cpp=.o} -c -std=c++17 -fPIC -g

clean:
		rm -rf $(OBJECTS)
		rm -rf $(USERINTERFACE_OBJECTS)
		rm -rf $(DEFAULTRENDERER_OBJECTS)
		rm -rf $(VIEWPORTRENDERER_OBJECTS)

fclean: clean
		rm -rf $(NAME)
		rm -rf $(PLUGINS_PATH)/$(USERINTERFACE_NAME)
		rm -rf $(PLUGINS_PATH)/$(DEFAULTRENDERER_NAME)
		rm -rf $(PLUGINS_PATH)/$(VIEWPORTRENDERER_NAME)

re: fclean all

