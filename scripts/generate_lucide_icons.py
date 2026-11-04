# 根据 lucide-static 的 codepoints.json 生成 ShadIconType.h 图标枚举
# 用法: python scripts/generate_lucide_icons.py [lucide 版本号]
import json
import pathlib
import re
import sys

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
PROJECT_DIR = SCRIPT_DIR.parent
CODEPOINTS_PATH = SCRIPT_DIR / "lucide-codepoints.json"
OUTPUT_PATH = PROJECT_DIR / "ShadcnWidgetTools" / "ShadIconType.h"
DEFAULT_VERSION = "1.61.0"


def to_pascal_case(name: str) -> str:
    # kebab-case 转 PascalCase，相邻两段都是数字时用下划线分隔（区分 0-1 与 01），数字开头时加 Icon 前缀
    result = ""
    for part in (item for item in name.split("-") if item):
        if result and result[-1].isdigit() and part[0].isdigit():
            result += "_"
        result += part[:1].upper() + part[1:]
    if not result or result[0].isdigit():
        result = "Icon" + result
    return result


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    version = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_VERSION
    with open(CODEPOINTS_PATH, "r", encoding="utf-8") as file:
        codepoints = json.load(file)

    entries = []
    used_names = {"None": "<reserved>"}
    for icon_name, codepoint in sorted(codepoints.items(), key=lambda item: item[0]):
        if not re.match(r"^[a-z0-9][a-z0-9-]*$", icon_name):
            print(f"非法图标名称: {icon_name}", file=sys.stderr)
            return 1
        enum_name = to_pascal_case(icon_name)
        if enum_name in used_names:
            print(f"枚举名称冲突: {icon_name} 与 {used_names[enum_name]} 都映射为 {enum_name}", file=sys.stderr)
            return 1
        if not 0 < int(codepoint) <= 0xFFFF:
            print(f"码位超出 BMP 范围: {icon_name} = {codepoint}", file=sys.stderr)
            return 1
        used_names[enum_name] = icon_name
        entries.append((enum_name, int(codepoint), icon_name))

    lines = [
        "// 本文件由 scripts/generate_lucide_icons.py 自动生成，请勿手动修改",
        f"// 图标来源: lucide-static {version} (ISC License)，共 {len(entries)} 个图标",
        "#ifndef SHADICONTYPE_H",
        "#define SHADICONTYPE_H",
        "",
        "namespace ShadIconType",
        "{",
        "enum IconName : unsigned int",
        "{",
        "    None = 0x0000,",
    ]
    for enum_name, codepoint, icon_name in entries:
        lines.append(f"    {enum_name} = 0x{codepoint:04X}, // {icon_name}")
    lines += [
        "};",
        "} // namespace ShadIconType",
        "",
        "#endif // SHADICONTYPE_H",
        "",
    ]

    with open(OUTPUT_PATH, "w", encoding="utf-8", newline="\n") as file:
        file.write("\n".join(lines))
    print(f"已生成 {OUTPUT_PATH}，共 {len(entries)} 个图标")
    return 0


if __name__ == "__main__":
    sys.exit(main())
