import json
import sys

def pack_json_as_strings(manifest_file, signature_file, output_file):
    # читаем файлы как строки, не парсим
    with open(manifest_file, "r", encoding="utf-8") as f:
        manifest_str = f.read()
    with open(signature_file, "r", encoding="utf-8") as f:
        signature_str = f.read()

    # создаем JSON с этими строками
    combined = {
        "manifest": manifest_str,
        "signature": signature_str
    }

    # записываем в файл
    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(combined, f, ensure_ascii=False, indent=2)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <manifest.json> <signature.json> <output.json>")
        sys.exit(1)

    pack_json_as_strings(sys.argv[1], sys.argv[2], sys.argv[3])
