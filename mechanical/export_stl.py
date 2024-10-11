#!/usr/bin/python3
import math
import os
import sys
from pathlib import Path

freecad_paths = [
    '/tmp/freecad/squashfs-root/usr/lib',
]

for path in freecad_paths:
    if os.path.exists(path):
        print(f"Added possible FreeCAD path: {path}")
        sys.path.append(path)

import FreeCAD
import MeshPart

def get_printbed_plane(body):
    """Get the Planz which should be used as the print bed for the body."""
    printbed = None
    flipped = False
    for sub_obj in body.OutList:
        if(sub_obj.TypeId == "App::Origin"):
            for origin in sub_obj.OutList:
                if("printbed" in origin.Label and "Plane" in origin.TypeId):
                    printbed = origin
                    flipped = "flipped" in origin.Label
                    return printbed, flipped
        if("printbed" in sub_obj.Label and "Plane" in sub_obj.TypeId):
            printbed = sub_obj
            flipped = "flipped" in sub_obj.Label
            return printbed, flipped
    raise ValueError("No printbed found in part!")

if __name__ == '__main__':
    cad_file = Path(sys.argv[1])
    doc = FreeCAD.open(str(cad_file.absolute()))

    # Define the parts we want to 3d print.
    parts = {
        'shell': None,
        'long_hub': None,
        'short_hub': None,
        'core': None
    }

    # Get the parts we want to 3d print.
    for obj in doc.Objects:
        if obj.TypeId == 'App::Part' and obj.Label in parts:
            matched_bodies = [sub_obj for sub_obj in obj.OutList if sub_obj.TypeId == "PartDesign::Body"]
            if len(matched_bodies) != 1:
                raise ValueError(f"Expected exactly 1 body in part!'{obj.Label}'")
            parts[obj.Label] = matched_bodies[0] if matched_bodies else None

    for name, body in parts.items():
        shape = body.Shape.copy(False)
        # Orient the part to the printbed.
        plane, flipped = get_printbed_plane(body)
        matrix = plane.Placement.Matrix.inverse()
        if flipped: # Flip around the printbed plane.
            matrix.rotateX(math.pi)
        shape = shape.transformShape(matrix)
        # Export as .stl file.
        mesh = doc.addObject("Mesh::Feature", "Mesh")
        mesh.Mesh = MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.01, AngularDeflection=0.025, Relative=False)
        mesh.Mesh.write(f"build/{name}.stl")

    FreeCAD.closeDocument(doc.Name)