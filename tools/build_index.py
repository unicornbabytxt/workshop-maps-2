#!/usr/bin/env python3
"""Turns data/maps.json into data/maps.tsv, the compact list the plugin downloads for its Browse tab.

Run from the repo root:  python3 tools/build_index.py
One map per line, tab separated: id, name, author, downloads, updated, tags (comma separated), image_url, description
"""
import json, re, unicodedata

REPLACEMENTS = {"‘": "'", "’": "'", "“": '"', "”": '"', "–": "-", "—": "-", "…": "...", " ": " "}

def clean(text, limit):
    text = str(text or "")
    for a, b in REPLACEMENTS.items():
        text = text.replace(a, b)
    # The in-game font only has basic characters, so fold accents and drop anything else.
    text = unicodedata.normalize("NFKD", text).encode("ascii", "ignore").decode("ascii")
    text = re.sub(r"\s+", " ", text).strip()
    if len(text) > limit:
        text = text[: limit - 3].rstrip() + "..."
    return text

def main():
    raw = open("data/maps.json", encoding="utf-8").read()
    maps = json.loads(raw[raw.find("["): raw.rfind("]") + 1])
    seen, lines = set(), []
    for m in maps:
        try:
            map_id = int(m["id"])
        except (KeyError, TypeError, ValueError):
            continue
        if map_id <= 0 or map_id in seen:
            continue
        seen.add(map_id)
        image = str(m.get("image_url") or "")
        if not image.startswith("https://cdn.bakkesplugins.com/"):
            image = ""
        updated = str(m.get("updated") or "")
        if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", updated):
            updated = ""
        tags = ",".join(clean(t, 24) for t in (m.get("tags") or []) if clean(t, 24))
        lines.append("\t".join([
            str(map_id), clean(m.get("name"), 60), clean(m.get("author"), 30),
            str(max(0, int(m.get("downloads") or 0))), updated, tags, image, clean(m.get("description"), 260),
        ]))
    open("data/maps.tsv", "w", encoding="ascii", newline="\n").write("\n".join(lines) + "\n")
    print(f"wrote {len(lines)} maps to data/maps.tsv")

if __name__ == "__main__":
    main()
