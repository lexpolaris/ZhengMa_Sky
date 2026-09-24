#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
把 Train.xml（旧 XML 题库）转换为紧凑的 TSV 题库 train.txt。

新格式（UTF-8，制表符分隔）：
  单元头:  @Lib<TAB>LibNo<TAB>LibName<TAB>Count
  题目行:  字词<TAB>编码<TAB>拆分<TAB>联想

说明:
  - 字段中若含制表符/换行，会被替换为空格（题库数据里不存在制表符）。
  - 空字段仍保留占位（连续的 \t），解析端按列取。
  - 行首以 '#' 开头视为注释。
"""
import xml.etree.ElementTree as ET
import sys

SRC = "Train.xml"
DST = "train.txt"


def clean(s: str) -> str:
    if s is None:
        return ""
    # 题库字段中不应出现制表符或换行
    return s.replace("\t", " ").replace("\n", " ").replace("\r", " ")


def main():
    tree = ET.parse(SRC)
    root = tree.getroot()

    libs = root.find("Libs")
    if libs is None:
        print("未找到 <Libs> 节点", file=sys.stderr)
        sys.exit(1)

    out = []
    out.append("# 郑码天空 题库（TSV）")
    out.append("# 单元头: @Lib<TAB>LibNo<TAB>LibName<TAB>Count")
    out.append("# 题目行: 字词<TAB>编码<TAB>拆分<TAB>联想")
    out.append("")

    unit_count = 0
    item_total = 0

    for lib in libs.findall("Lib"):
        lib_no = lib.get("LibNo", "0")
        lib_name = clean(lib.get("LibName", ""))
        items = lib.findall("b")
        count = len(items)

        out.append(f"@Lib\t{lib_no}\t{lib_name}\t{count}")
        for b in items:
            c = clean(b.get("c", ""))
            m = clean(b.get("m", ""))
            s = clean(b.get("s", ""))
            a = clean(b.get("a", ""))
            out.append(f"{c}\t{m}\t{s}\t{a}")
        out.append("")

        unit_count += 1
        item_total += count

    with open(DST, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(out))

    print(f"转换完成: {unit_count} 个单元, {item_total} 道题 -> {DST}")


if __name__ == "__main__":
    main()
