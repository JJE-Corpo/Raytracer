# Vertex Editing

Move the individual vertices of an editable primitive directly in the viewport,
by dragging on-screen handles or typing coordinates. It is the interactive
counterpart to the object transform (position/rotation/scale) exposed in the
Object panel.

## What is editable

A primitive is editable when it implements
[`IEditablePrimitive`](../srcs/common/scene/IEditablePrimitive.hpp) in addition
to `IPrimitive`. The UI discovers the capability at runtime with
`dynamic_cast<IEditablePrimitive *>(primitive)` — the same runtime-capability
pattern the renderer uses for selection awareness — so `IPrimitive` (the plugin
contract) is left untouched.

Currently editable:

| Primitive | Vertices |
|---|---|
| **Triangle** | its 3 corners |
| **Mesh** | the unique (welded) vertices of the loaded `.obj`; a shared vertex moves every incident face |

## Entering / leaving edit mode

- **Enter**: select a single editable primitive, then press **Tab** — or
  **double-click** it in the viewport.
- **Leave**: press **Tab** again or **Escape**. Selecting a different object (in
  the hierarchy or by deleting the edited one) also leaves edit mode.
- While in edit mode a banner **“Edit Mode — &lt;name&gt;”** is shown over the
  viewport and a toast confirms the transition.

Classic object picking in the viewport is disabled in edit mode so a click never
selects a different object by accident. Right-drag still orbits the camera; the
WASD/ZQSD fly keys stay available except during an active vertex drag (so the
axis-lock keys below don't also move the camera).

## Handles, hovering and picking

Every vertex is projected to screen and drawn as a small circle:

- **Blue** — idle vertex.
- **White** — the vertex under the cursor.
- **Yellow** — the selected vertex.

Clicking picks the vertex whose handle is nearest the cursor within a small pixel
radius; ties (overlapping handles) go to the vertex closest to the camera.
Vertices behind the camera or off screen are skipped. For very dense meshes the
number of drawn handles is capped (picking still considers them all).

## Dragging a vertex

Drag a handle to move its vertex. By default the vertex slides in the plane that
passes through its starting position and is **parallel to the screen** (normal =
camera forward) — the intuitive "move it where I point" behaviour.

- **Axis lock**: hold **X**, **Y** or **Z** during the drag to constrain the
  move to that world axis.
- **Keyboard**: the selected vertex's world coordinates appear in a **Vertex**
  field in the Object panel; typing a value moves the vertex and stays in sync
  with dragging.

After every edit the scene BVH is flagged dirty and the viewport re-traces, so
the change is visible immediately.

### Mesh specifics

- Moving a shared vertex updates **all incident faces**.
- The affected face normals are recomputed, and smooth (interpolated) vertex
  normals are refreshed as the average of the incident face normals.
- The mesh's **local BVH** is rebuilt when the vertex is released
  (`onGeometryChanged`). For small meshes it is also rebuilt live during the
  drag; for large meshes (> ~4000 triangles) the surface renders slightly stale
  during the drag and snaps to the final shape on release, which keeps dragging
  responsive.

## Saving edits (serialization)

- **Triangle** — its three corners are already part of the scene JSON, so edited
  positions are saved and reloaded verbatim.
- **Mesh** — the `.obj` file is never modified. Edits are stored as
  **object-space overrides** in a `vertex_overrides` list on the mesh object and
  re-applied after the `.obj` is loaded:

  ```json
  {
      "name": "Cube",
      "type": "mesh",
      "file": "tests/obj/cube.obj",
      "position": [26.0, 10.0, 15.0],
      "rotation": [0.0, 0.0, 20.0],
      "scale": [14.0, 14.0, 14.0],
      "material": "Blue",
      "vertex_overrides": [
          { "index": 0, "position": [1.0, -1.0, 0.857] }
      ]
  }
  ```

  Because overrides are object-space, they survive later changes to the mesh
  transform and round-trip cleanly: **load → edit → save → reload** restores the
  edited shape.

See [`tests/configs/vertex_edit.json`](../tests/configs/vertex_edit.json) for a
scene with a Triangle and a small cube mesh set up to try this out.

## Limitations

- Editing changes geometry only; it does not add or remove vertices/faces.
- Smooth normals recomputed at edited vertices are geometric averages, so hard
  edges authored in the `.obj` (`vn`) are softened locally where you edit.
- The move plane is view-parallel; there is no separate depth-along-view drag
  (use the Vertex field or an axis lock for precise placement).
