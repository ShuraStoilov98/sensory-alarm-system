#!/usr/bin/env python3
"""Render the original 3MF geometry as a labeled CAD turntable, not a demo.

Optional tooling: numpy==2.3.3 Pillow==11.3.0 imageio-ffmpeg==0.6.0
Run from any directory: python3 tools/render_cad.py
"""

import math
from pathlib import Path
import posixpath
import xml.etree.ElementTree as ET
import zipfile

import imageio_ffmpeg
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
NS = {"m": "http://schemas.microsoft.com/3dmanufacturing/core/2015/02"}
PRODUCTION = "{http://schemas.microsoft.com/3dmanufacturing/production/2015/06}"
SIZE = (1280, 720)
FRAMES = 96
FPS = 16
PARTS = [
    ("CurtainPuller_v2.3mf", "Curtain puller", (51, 76, 104)),
    ("ESP32-DRV-BTNs_Holder.3mf", "Controller holder", (37, 105, 118)),
    ("Button_holders.3MF", "Button holder", (81, 92, 109)),
]


def transform(vertices, value):
    if not value:
        return vertices
    matrix = np.array([float(x) for x in value.split()]).reshape(4, 3)
    return vertices @ matrix[:3] + matrix[3]


def load_model(path):
    """Resolve 3MF build items/components, including external objects/transforms."""
    with zipfile.ZipFile(path) as archive:
        models = {}

        def document(name):
            if name not in models:
                models[name] = ET.fromstring(archive.read(name))
            return models[name]

        def object_mesh(name, identifier):
            obj = document(name).find(f'm:resources/m:object[@id="{identifier}"]', NS)
            if obj is None:
                raise ValueError(f"Missing object {identifier} in {name}")
            mesh = obj.find("m:mesh", NS)
            if mesh is not None:
                vertices = np.array([
                    [float(v.attrib[k]) for k in ("x", "y", "z")]
                    for v in mesh.findall("m:vertices/m:vertex", NS)
                ])
                triangles = np.array([
                    [int(t.attrib[k]) for k in ("v1", "v2", "v3")]
                    for t in mesh.findall("m:triangles/m:triangle", NS)
                ])
                return vertices, triangles
            chunks = []
            for component in obj.findall("m:components/m:component", NS):
                target = component.get(PRODUCTION + "path", name)
                target = target.lstrip("/") if target.startswith("/") else (
                    name if target == name else posixpath.normpath(
                        posixpath.join(posixpath.dirname(name), target)))
                vertices, triangles = object_mesh(target, component.attrib["objectid"])
                chunks.append((transform(vertices, component.get("transform")), triangles))
            return combine(chunks)

        name = "3D/3dmodel.model"
        chunks = []
        for item in document(name).findall("m:build/m:item", NS):
            vertices, triangles = object_mesh(name, item.attrib["objectid"])
            chunks.append((transform(vertices, item.get("transform")), triangles))
        vertices, triangles = combine(chunks)
    vertices -= (vertices.min(axis=0) + vertices.max(axis=0)) / 2
    vertices /= np.linalg.norm(vertices, axis=1).max()
    return vertices, triangles


def combine(chunks):
    if not chunks:
        raise ValueError("3MF contains no renderable mesh")
    vertices, triangles, offset = [], [], 0
    for points, faces in chunks:
        vertices.append(points)
        triangles.append(faces + offset)
        offset += len(points)
    return np.concatenate(vertices), np.concatenate(triangles)


def font(size, bold=False):
    names = [
        Path("/usr/share/fonts/truetype/dejavu") / (
            "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"),
        Path("C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf"),
        Path("/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else
             "/System/Library/Fonts/Supplemental/Arial.ttf"),
    ]
    for name in names:
        if name.exists():
            return ImageFont.truetype(str(name), size)
    return ImageFont.load_default(size=size)


def frame(models, index):
    image = Image.new("RGB", SIZE, "#f8fafc")
    draw = ImageDraw.Draw(image)
    draw.text((48, 32), "SENSORY ALARM / ORIGINAL CAD", fill="#183045", font=font(30, True))
    draw.text((48, 80), "Three printable parts from the curtain installation",
              fill="#546477", font=font(20))
    angle = 0.65 + 2 * math.pi * index / FRAMES
    yaw = np.array([[math.cos(angle), -math.sin(angle), 0],
                    [math.sin(angle), math.cos(angle), 0], [0, 0, 1]])
    elevation = 0.5
    camera = np.array([[1, 0, 0], [0, math.cos(elevation), -math.sin(elevation)],
                       [0, math.sin(elevation), math.cos(elevation)]])
    light = np.array([0.2, -0.6, 0.8])
    light /= np.linalg.norm(light)
    for column, ((vertices, triangles), (_, title, color)) in enumerate(zip(models, PARTS)):
        x = 48 + column * 404
        draw.rounded_rectangle((x, 130, x + 376, 597), radius=16,
                               fill="white", outline="#dce4eb", width=2)
        draw.text((x + 22, 153), f"0{column + 1} / {title}", fill="#183045", font=font(20, True))
        draw.ellipse((x + 83, 493, x + 293, 520), fill="#edf1f5")
        points = vertices @ yaw @ camera
        faces = points[triangles]
        normals = np.cross(faces[:, 1] - faces[:, 0], faces[:, 2] - faces[:, 0])
        lengths = np.linalg.norm(normals, axis=1)
        normals /= np.maximum(lengths[:, None], 1e-12)
        brightness = 0.55 + 0.6 * np.abs(normals @ light)
        colors = np.clip(np.array(color)[None, :] * brightness[:, None] + 22, 0, 255).astype(int)
        screen = np.column_stack((x + 188 + points[:, 0] * 148, 365 - points[:, 2] * 148))
        for face in np.argsort(faces[:, :, 1].mean(axis=1))[::-1]:
            draw.polygon([tuple(p) for p in screen[triangles[face]]], fill=tuple(colors[face]))
        draw.text((x + 22, 554), "Original 3MF geometry", fill="#546477", font=font(18))
    draw.text((48, 632), "CAD TURNTABLE — NOT OPERATING FOOTAGE", fill="#183045", font=font(21, True))
    draw.text((48, 670), "Rotating model views. No motion simulation or hardware validation is implied.",
              fill="#546477", font=font(17))
    return image


def main():
    models = [load_model(ROOT / "mechanical parts" / part[0]) for part in PARTS]
    output = ROOT / "docs/images"
    output.mkdir(parents=True, exist_ok=True)
    writer = imageio_ffmpeg.write_frames(
        str(output / "cad-turntable.mp4"), SIZE, fps=FPS, codec="libx264",
        output_params=["-crf", "22", "-movflags", "+faststart"], ffmpeg_log_level="error",
    )
    writer.send(None)
    gif_frames = []
    try:
        for index in range(FRAMES):
            image = frame(models, index)
            writer.send(np.asarray(image))
            # Half-rate GIF for GitHub; the downloadable video keeps all frames.
            if index % 2 == 0:
                small = image.resize((768, 432), Image.Resampling.LANCZOS)
                gif_frames.append(small.quantize(colors=128))
    finally:
        writer.close()
    gif_frames[0].save(output / "cad-turntable.gif", save_all=True,
                       append_images=gif_frames[1:], duration=125, loop=0, optimize=True)
    print("Rendered", [(part[1], len(mesh[1])) for part, mesh in zip(PARTS, models)])
    print("Saved docs/images/cad-turntable.mp4 and docs/images/cad-turntable.gif")


if __name__ == "__main__":
    main()
