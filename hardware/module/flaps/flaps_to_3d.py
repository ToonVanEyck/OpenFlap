#!/opt/FreeCAD/usr/bin/python
import os
import sys
import argparse
from colorama import Fore, Style
import zipfile
from lxml import etree
from itertools import combinations

freecad_paths = [
    "/opt/FreeCAD/usr/lib",
    "/opt/FreeCAD/usr/lib/python3.11/site-packages",
]

for path in freecad_paths:
    if os.path.exists(path):
        sys.path.append(path)

import FreeCAD
from BOPTools import BOPFeatures
import importSVG
import MeshPart


def svg_set_path_ids(svg_file):
    parser = etree.XMLParser(remove_blank_text=True)
    tree = etree.parse(svg_file, parser)
    root = tree.getroot()

    namespaces = {
        "svg": "http://www.w3.org/2000/svg",
        "inkscape": "http://www.inkscape.org/namespaces/inkscape",
    }

    for group in root.findall(".//svg:g[@inkscape:label]", namespaces):
        label = group.attrib[
            "{http://www.inkscape.org/namespaces/inkscape}label"
        ].replace(" ", "_")
        for index, path in enumerate(group.findall(".//svg:path", namespaces)):
            path.attrib["id"] = f"{label}_{index}"

    tree.write(svg_file, pretty_print=True, xml_declaration=True, encoding="UTF-8")


def extrude_path(doc, pathObj, length, reversed=False, offset=0):
    extrudeName = f"{pathObj.Name}_extrude"
    extrudeObj = doc.addObject("Part::Extrusion", extrudeName)
    extrudeObj.Base = pathObj
    extrudeObj.DirMode = "Custom"
    extrudeObj.Dir = FreeCAD.Vector(
        0.000000000000000, 0.000000000000000, 1.000000000000000
    )
    extrudeObj.DirLink = None
    extrudeObj.LengthFwd = length
    extrudeObj.LengthRev = 0.000000000000000
    extrudeObj.Solid = True
    extrudeObj.Reversed = reversed
    extrudeObj.Symmetric = False
    extrudeObj.TaperAngle = 0.000000000000000
    extrudeObj.TaperAngleRev = 0.000000000000000
    extrudeObj.Placement = FreeCAD.Placement(
        FreeCAD.Vector(0, 0, offset), FreeCAD.Rotation(FreeCAD.Vector(0, 0, 1), 0)
    )
    return extrudeObj


def combine_extrudes(doc, originalPaths):
    # When importing an svg into freecad, freecad imports sub-paths as a separate object.
    # For example, an "O" character will be imported as 2 separate paths. One for the outer circle and one for the inner circle.
    # The same is the case for characters like "?", "!" etc.
    # We need to combine the paths into a single path. But we need to determine whether to combine or cut the paths.
    bp = BOPFeatures.BOPFeatures(doc)
    combineObjs = set()
    cutObjs = set()

    if len(originalPaths) == 1:
        return [originalPaths[0]]

    # Get the non intersecting and intersecting objects.
    for obj1, obj2 in combinations(originalPaths, 2):
        intersectVolume = obj1.Shape.common(obj2.Shape).Volume
        if intersectVolume > 0:
            if intersectVolume == obj1.Shape.Volume:
                combineObjs.add(obj2)
                cutObjs.add(obj1)
            elif intersectVolume == obj2.Shape.Volume:
                combineObjs.add(obj1)
                cutObjs.add(obj2)
            else:
                print(
                    Fore.RED
                    + "ERROR: intersectVolume is not equal to any of the volumes"
                    + Style.RESET_ALL
                )
                exit(1)
        else:
            combineObjs.add(obj1)
            combineObjs.add(obj2)

    if len(combineObjs) == 0:
        return []

    # combine the objects
    newObjs = combineObjs.pop()
    for obj in combineObjs:
        newObjs = bp.make_fuse([newObjs.Name, obj.Name])
    for obj in cutObjs:
        newObjs = bp.make_cut([newObjs.Name, obj.Name])
    return [newObjs]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert flap SVG files to 3D models using FreeCAD."
    )
    parser.add_argument(
        "zip_path", help="Path to the input .zip file containing flap SVG files."
    )
    parser.add_argument(
        "output_dir", help="Directory where the output flap 3D models will be saved."
    )
    args = parser.parse_args()

    # Constants
    EXTRACT_PATH = "/tmp/flaps"
    FLAP_THICKNESS_MM = 0.75  # Thickness of the flap
    COLOR_THICKNESS_MM = 0.15 * 2  # Thickness of the colored characters.

    # Printing the the flaps with 0.15 mm layers will result 5 layers (0.75 mm / 0.15 mm = 5)
    # The colored characters will be printed with 2 layers (0.15 mm * 2 = 0.30 mm)
    # This leaves a solid black layer in the middle.

    # Create the EXTRACT_PATH directory if it does not exist or clear it if it does
    if not os.path.exists(EXTRACT_PATH):
        os.makedirs(EXTRACT_PATH)
    else:
        for file in os.listdir(EXTRACT_PATH):
            file_path = os.path.join(EXTRACT_PATH, file)
            if os.path.isfile(file_path):
                os.unlink(file_path)

    # Extract the SVG files from the zip archive
    with zipfile.ZipFile(args.zip_path, "r") as zip_ref:
        zip_ref.extractall(EXTRACT_PATH)

    # Sort files
    svg_files = sorted(
        [file for file in os.listdir(EXTRACT_PATH) if file.endswith(".svg")],
        key=lambda x: int(x.split("_")[-1].split(".")[0]),
    )

    for svg_file in svg_files:
        file_basename = svg_file.split(".")[0]
        file_path = os.path.join(EXTRACT_PATH, svg_file)

        print(Fore.BLUE + f"converting {file_basename} to 3d" + Style.RESET_ALL)

        # Set the paths ids in the SVG file
        svg_set_path_ids(file_path)

        # Create a new FreeCAD document for the SVG file
        doc = FreeCAD.newDocument(f"flap")
        importSVG.insert(str(file_path), doc.Name)

        # Remove all objects except the top_silk, bottom_silk, and outline paths
        top_silk_paths = []
        bot_silk_paths = []
        outline_path = None
        for obj in doc.Objects:
            if "top_silk" in obj.Name:
                top_silk_paths.append(obj)
            elif "bottom_silk" in obj.Name:
                bot_silk_paths.append(obj)
            elif "outline" in obj.Name:
                if outline_path is not None:
                    print(
                        Fore.RED
                        + "Multiple outline paths found in the SVG file."
                        + Style.RESET_ALL
                    )
                    exit()
                else:
                    outline_path = obj
            else:
                doc.removeObject(obj.Name)

        # Extrude the outline and the silk paths
        outlineExtrudeObj = extrude_path(doc, outline_path, FLAP_THICKNESS_MM, False)
        topExtrudeObjs = [
            extrude_path(doc, path, COLOR_THICKNESS_MM, True, FLAP_THICKNESS_MM)
            for path in top_silk_paths
        ]
        botExtrudeObjs = [
            extrude_path(doc, path, COLOR_THICKNESS_MM, False)
            for path in bot_silk_paths
        ]

        # Recompute the document required before performing boolean operations
        doc.recompute()

        # Some characters Like an "O","B","8" have holes in them. They are imported as separate paths. So we need to cut the inner paths out of the outer path.
        # Also, characters like "?", "!" have multiple paths. We need to combine them into a single path.
        correctedExtrudeObjs = combine_extrudes(doc, topExtrudeObjs) + combine_extrudes(
            doc, botExtrudeObjs
        )

        # Generate a list of cut tools by copying the extruded paths
        cutToolObjs = [doc.copyObject(obj) for obj in correctedExtrudeObjs]

        # Recompute the document required before performing boolean operations
        doc.recompute()

        # Perform a boolean cut operation to remove the copied paths from the outline
        bp = BOPFeatures.BOPFeatures(doc)
        flapObj = outlineExtrudeObj
        for cutToolObj in cutToolObjs:
            flapObj = bp.make_cut([flapObj.Name, cutToolObj.Name])

        # Combine the shapes of topExtrudeObjs and botExtrudeObjs
        combinedObj = doc.addObject("Part::Compound", "combined")
        combinedObj.Links = [flapObj] + correctedExtrudeObjs

        # Recompute the document
        doc.recompute()

        # Export the final object as a .stl file
        shape = combinedObj.Shape.copy(False)
        mesh = doc.addObject("Mesh::Feature", "Mesh")
        mesh.Mesh = MeshPart.meshFromShape(
            Shape=shape,
            LinearDeflection=0.01,
            AngularDeflection=0.025,
        )
        mesh_file = os.path.join(args.output_dir, f"{file_basename}.stl")
        mesh.Mesh.write(mesh_file)

        # Close the FreeCAD document
        FreeCAD.closeDocument(doc.Name)

        # Save the FreeCAD document as a .FCStd file
        # output_file = os.path.join(args.output_dir, f"{file_basename}.FCStd")
        # doc.saveAs(str(output_file))
        # exit()
