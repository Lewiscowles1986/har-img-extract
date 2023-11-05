import json
import base64
import os

# list of supported image mime-types
# Special thanks to https://gist.github.com/FurloSK/0477e01024f701db42341fc3223a5d8c
# Special mention, and thanks to MDN
# https://developer.mozilla.org/en-US/docs/Web/HTTP/Basics_of_HTTP/MIME_types/Common_types 
mimetypes = {
    "image/webp": ".webp",
    "image/jpeg": ".jpeg", # *.jpg files have two possible extensions
    "image/jpeg": ".jpg",  #   (but .jpeg is official and thus preferred)
    "image/png": ".png",
    "image/svg+xml": ".svg",
    "image/avif": ".avif",
    "image/bmp": ".bmp",
    "image/gif": ".gif",
    "image/vnd.microsoft.icon": ".ico",
    "image/tiff": ".tif",  # *.tiff files have two possible extensions
    "image/tiff": ".tiff", #   (but .tiff is what I know and prefer)
}
# make sure the output directory exists before running!
folder = os.path.join(os.getcwd(), "imgs")

with open("src.har", "r") as f:
    har = json.loads(f.read())

entries = har["log"]["entries"]

for entry in entries:
    mimetype = entry["response"]["content"]["mimeType"]
    filename = entry["request"]["url"].split("/")[-1]
    image64 = entry["response"]["content"]["text"]

    # Python lets you lookup values against dictionaries using the in keyword
    if mimetype in mimetypes:
        ext = mimetypes[mimetype]
        file = os.path.join(folder, f"{filename}.{ext}")
        print(file)
        with open(file, "wb") as f:
            f.write(base64.b64decode(image64))
