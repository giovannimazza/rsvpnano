Import("env")

import hashlib
from pathlib import Path


PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
DATA_BOOKS_DIR = PROJECT_DIR / "data" / "books" / "books"
GENERATED_HEADER = PROJECT_DIR / "src" / "storage" / "SeedBooks.generated.h"


def escape_raw_string(contents: str) -> tuple[str, str]:
    for index in range(8):
        delimiter = "SEEDBOOKS" + ("_" * index)
        if f"){delimiter}\"" not in contents:
            return delimiter, contents
    raise SystemExit("Could not find a safe raw-string delimiter for seed books.")


def load_seed_books() -> list[tuple[str, str]]:
    if not DATA_BOOKS_DIR.exists():
        return []

    books: list[tuple[str, str]] = []
    for path in sorted(DATA_BOOKS_DIR.glob("*.rsvp")):
      books.append((f"/books/books/{path.name}", path.read_text(encoding="utf-8")))
    for path in sorted(DATA_BOOKS_DIR.glob("*.txt")):
      books.append((f"/books/books/{path.name}", path.read_text(encoding="utf-8")))
    return books


def manifest_hash(books: list[tuple[str, str]]) -> str:
    digest = hashlib.sha256()
    for path, contents in books:
        digest.update(path.encode("utf-8"))
        digest.update(b"\0")
        digest.update(contents.encode("utf-8"))
        digest.update(b"\0")
    return digest.hexdigest()


def render_header(books: list[tuple[str, str]]) -> str:
    hash_value = manifest_hash(books)
    lines = [
        "#pragma once",
        "",
        "#include <cstddef>",
        "",
        "struct SeededBook {",
        "  const char *path;",
        "  const char *contents;",
        "};",
        "",
        "constexpr const char kSeedManifestHash[] = \"" + hash_value + "\";",
        "",
    ]

    if books:
        lines.append("constexpr SeededBook kSeededBooks[] = {")
        for path, contents in books:
            delimiter, raw_contents = escape_raw_string(contents)
            lines.append(f'  {{ "{path}", R"{delimiter}({raw_contents}){delimiter}" }},')
        lines.append("};")
        lines.append("constexpr std::size_t kSeedBookCount = sizeof(kSeededBooks) / sizeof(kSeededBooks[0]);")
    else:
        lines.append("constexpr SeededBook kSeededBooks[] = {};")
        lines.append("constexpr std::size_t kSeedBookCount = 0;")

    lines.append("")
    return "\n".join(lines)


books = load_seed_books()
GENERATED_HEADER.parent.mkdir(parents=True, exist_ok=True)
GENERATED_HEADER.write_text(render_header(books), encoding="utf-8")
