import json
import base64
import os

# make sure the output directory exists before running!
folder = os.path.join(os.getcwd(), "imgs")

with open("src.har", "r") as f:
    har = json.loads(f.read())

entries = har["log"]["entries"]

for entry in entries:
    mimetype = entry["response"]["content"]["mimeType"]
    filename = entry["request"]["url"].split("/")[-1]
    image64 = entry["response"]["content"]["text"]

    if any([
        mimetype == "image/webp",
        mimetype == "image/jpeg",
        mimetype == "image/png"
    ]):
        ext = {
            "image/webp": "webp",
            "image/jpeg": "jpg",
            "image/png": "png",
        }.get(mimetype)
        file = os.path.join(folder, f"{filename}.{ext}")
        print(file)
        with open(file, "wb") as f:
            f.write(base64.b64decode(image64))
