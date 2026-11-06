#!/usr/bin/env python3
"""Turn the retail SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE of a PKT 3.1 capture into rows of the RetailSystems
BattlePay catalog (retail_battlepay_group / _product / _deliverable / _product_bundle).

The layout read here is the one BattlePayPackets.cpp writes; a capture whose payload is not consumed exactly is
rejected. Opcode values come from the server's Opcodes.h, like extract_capture_catalog.py.

Deliveries the server can grant (items, mounts, toys) become active deliverables; services (boosts, game time,
character services, currencies) stay in the catalog for display but get no server delivery.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import struct
import sys

# retail deliverable type -> server delivery_type (0 item, 1 spell, 2 mount, 3 toy)
ITEM_TYPES = {2, 4, 13, 26}   # battle pet cage, item, ensemble, transmog set: the item is granted
MOUNT_TYPE = 3
TOY_TYPE = 14


def server_opcode(name: str) -> int:
    header = pathlib.Path(__file__).resolve().parents[4] / "Server" / "Protocol" / "Opcodes.h"
    match = re.search(r"^\s+" + name + r"\s*=\s*(0x[0-9A-Fa-f]+)", header.read_text(encoding="latin-1"), re.M)
    if not match:
        raise ValueError(f"{name} not found in {header}")
    return int(match.group(1), 16)


def records(path: pathlib.Path):
    data = path.read_bytes()
    if data[:3] != b"PKT" or struct.unpack_from("<H", data, 3)[0] != 0x0301:
        raise ValueError("not a PKT 3.1 capture")
    offset = 5 + 1 + 4 + 4 + 40 + 4 + 4
    optional, = struct.unpack_from("<I", data, offset)
    offset += 4 + optional
    while offset + 20 <= len(data):
        direction = data[offset:offset + 4]
        _connection, _ticks, extra, size = struct.unpack_from("<IIII", data, offset + 4)
        offset += 20 + extra
        payload = data[offset:offset + size]
        offset += size
        if len(payload) >= 4:
            yield direction, struct.unpack_from("<I", payload)[0], payload[4:]


class Reader:
    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0
        self.bit = 0
        self.current = 0

    def flush(self):
        self.bit = 0

    def u8(self):
        self.flush()
        value = self.data[self.offset]
        self.offset += 1
        return value

    def u32(self):
        self.flush()
        value, = struct.unpack_from("<I", self.data, self.offset)
        self.offset += 4
        return value

    def u64(self):
        self.flush()
        value, = struct.unpack_from("<Q", self.data, self.offset)
        self.offset += 8
        return value

    def bits(self, count: int) -> int:
        value = 0
        for _ in range(count):
            if self.bit == 0:
                self.current = self.data[self.offset]
                self.offset += 1
                self.bit = 8
            self.bit -= 1
            value = (value << 1) | ((self.current >> self.bit) & 1)
        return value

    def string(self, size: int) -> str:
        self.flush()
        value = self.data[self.offset:self.offset + size]
        self.offset += size
        return value.decode("utf-8", "replace").rstrip("\x00")


def read_display(r: Reader) -> dict:
    has_file, has_scene = r.bits(1), r.bits(1)
    lengths = [r.bits(10), r.bits(10), r.bits(13), r.bits(13), r.bits(13)]
    optionals = [r.bits(1) for _ in range(4)]
    disclaimer_length, nydus_length = r.bits(13), r.bits(12)
    r.flush()
    cards = r.u32()
    display = {"card_type": r.u32(), "banner_type": r.u32(), "item_quantity": r.u32()}
    display["file"] = r.u32() if has_file else None
    display["scene"] = r.u32() if has_scene else None
    display["name1"], display["name2"], display["name3"], display["tooltip"], display["instructions"] = (
        r.string(length) for length in lengths)
    display["flags"], display["text_color"], display["texture"], display["background"] = (
        r.u32() if present else None for present in optionals)
    display["disclaimer"] = r.string(disclaimer_length)
    display["nydus"] = r.string(nydus_length)
    for _ in range(cards):
        title_length = r.bits(10)
        r.flush()
        r.u32(); r.u32(); r.u32()
        r.string(title_length)
    return display


def read_choice(r: Reader):
    for _ in range(6):
        r.u32()
    r.bits(1)
    has_pet, has_display = r.bits(1), r.bits(1)
    if has_pet:
        r.bits(4)
    r.flush()
    if has_display:
        read_display(r)


def read_catalog(payload: bytes) -> dict:
    r = Reader(payload)
    result, currency, product_count, deliverable_count, group_count, shop_count = (r.u32() for _ in range(6))
    if result != 0:
        raise ValueError(f"product list result {result}")

    products = []
    for _ in range(product_count):
        product = {"id": r.u32(), "normal_price": r.u64(), "current_price": r.u64()}
        deliverable_ids = r.u32()
        product["type"], product["flags"], product["required"] = r.u32(), r.u32(), r.u32()
        bundled = r.u32()
        product["eligibility"], product["pmt"] = r.u32(), r.u64()
        product["deliverables"] = [r.u32() for _ in range(deliverable_ids)]
        product["bundled"] = [r.u32() for _ in range(bundled)]
        has_display = r.bits(1)
        r.flush()
        product["display"] = read_display(r) if has_display else None
        products.append(product)

    deliverables = []
    for _ in range(deliverable_count):
        names = ("id", "type", "item", "quantity", "mount_spell", "pet_creature", "boost", "flags", "appearance",
                 "transmog_set", "title", "enchant", "warband_scene")
        deliverable = {name: r.u32() for name in names}
        name_length = r.bits(8)
        r.bits(1)
        has_pet, choices, has_display = r.bits(1), r.bits(7), r.bits(1)
        if has_pet:
            r.bits(4)
        r.flush()
        for _ in range(choices):
            read_choice(r)
        deliverable["name"] = r.string(name_length)
        deliverable["display"] = read_display(r) if has_display else None
        deliverables.append(deliverable)

    groups = []
    for _ in range(group_count):
        group = {"id": r.u32(), "icon": r.u32(), "display_type": r.u8(), "ordering": r.u32(), "flags": r.u32(),
                 "parent": r.u32()}
        name_length, disabled_length = r.bits(8), r.bits(24)
        r.flush()
        group["name"], group["disabled"] = r.string(name_length), r.string(disabled_length)
        groups.append(group)

    shops = []
    for _ in range(shop_count):
        shop = {"id": r.u32(), "group": r.u32(), "product": r.u32(), "ordering": r.u32(), "flags": r.u32(),
                "banner": r.u8()}
        has_display = r.bits(1)
        r.flush()
        shop["display"] = read_display(r) if has_display else None
        shops.append(shop)

    if r.offset != len(payload):
        raise ValueError(f"layout mismatch: {r.offset} of {len(payload)} bytes read")
    return {"currency": currency, "products": products, "deliverables": deliverables, "groups": groups, "shops": shops}


def sql(value) -> str:
    if value is None:
        return "NULL"
    if isinstance(value, str):
        return "'" + value.replace("\\", "\\\\").replace("'", "''") + "'"
    return str(value)


def write_sql(catalog: dict, build: str, out) -> None:
    shop_by_product = {}
    for shop in sorted(catalog["shops"], key=lambda s: (s["group"], s["ordering"])):
        shop_by_product.setdefault(shop["product"], shop)
    product_by_deliverable = {}
    for product in catalog["products"]:
        for deliverable_id in product["deliverables"]:
            product_by_deliverable.setdefault(deliverable_id, product["id"])

    product_ids = ", ".join(str(p["id"]) for p in catalog["products"])
    group_ids = ", ".join(str(g["id"]) for g in catalog["groups"])
    deliverable_ids = ", ".join(str(d["id"]) for d in catalog["deliverables"])

    out.write(f"-- RetailSystems BattlePay: the retail shop catalog of a {build} capture (product_list_to_sql.py):\n")
    out.write(f"-- {len(catalog['groups'])} groups, {len(catalog['products'])} products, "
              f"{len(catalog['deliverables'])} deliverables. Items, mounts and toys are delivered by the server;\n")
    out.write("-- services (boosts, game time, character services, currencies) are listed without a delivery.\n\n")

    out.write(f"DELETE FROM `retail_battlepay_group` WHERE `group_id` IN ({group_ids});\n")
    out.write("INSERT INTO `retail_battlepay_group` (`group_id`, `icon_file_data_id`, `display_type`, `ordering`, `flags`, "
              "`parent_group_id`, `name`, `disabled_description`, `active`) VALUES\n")
    out.write(",\n".join(
        f"({g['id']}, {g['icon']}, {g['display_type']}, {g['ordering']}, {g['flags']}, {g['parent']}, {sql(g['name'])}, "
        f"{sql(g['disabled'])}, 1)" for g in catalog["groups"]) + ";\n\n")

    out.write(f"DELETE FROM `retail_battlepay_product` WHERE `product_id` IN ({product_ids});\n")
    out.write("INSERT INTO `retail_battlepay_product` (`product_id`, `group_id`, `normal_price`, `current_price`, "
              "`product_type`, `flags`, `required_deliverable_id`, `eligibility`, `pmt_product_id`, `ordering`, `shop_flags`, "
              "`banner_type`, `display_file_data_id`, `display_model_scene_id`, `name_1`, `name_2`, `name_3`, `tooltip`, "
              "`instructions`, `display_flags`, `override_text_color`, `override_texture`, `override_background`, `disclaimer`, "
              "`nydus_link`, `battlepay_card_type`, `item_quantity`, `active`) VALUES\n")
    rows = []
    for p in catalog["products"]:
        shop = shop_by_product.get(p["id"])
        d = p["display"] or (shop["display"] if shop else None) or {}
        rows.append(
            f"({p['id']}, {shop['group'] if shop else 0}, {p['normal_price']}, {p['current_price']}, {p['type']}, "
            f"{p['flags']}, {p['required']}, {p['eligibility']}, {p['pmt']}, {shop['ordering'] if shop else 0}, "
            f"{shop['flags'] if shop else 0}, {shop['banner'] if shop else 0}, {sql(d.get('file'))}, {sql(d.get('scene'))}, "
            f"{sql(d.get('name1'))}, {sql(d.get('name2'))}, {sql(d.get('name3'))}, {sql(d.get('tooltip'))}, "
            f"{sql(d.get('instructions'))}, {sql(d.get('flags'))}, {sql(d.get('text_color'))}, {sql(d.get('texture'))}, "
            f"{sql(d.get('background'))}, {sql(d.get('disclaimer'))}, {sql(d.get('nydus'))}, {d.get('card_type', 0)}, "
            f"{d.get('item_quantity', 0)}, {1 if shop else 0})")
    out.write(",\n".join(rows) + ";\n\n")

    out.write(f"DELETE FROM `retail_battlepay_deliverable` WHERE `deliverable_id` IN ({deliverable_ids});\n")
    out.write("INSERT INTO `retail_battlepay_deliverable` (`deliverable_id`, `product_id`, `client_type`, `delivery_type`, "
              "`entry`, `quantity`, `flags`, `name`, `item_id`, `mount_spell_id`, `battle_pet_creature_id`, `boost_id`, "
              "`trans_item_modified_appearance_id`, `transmog_set_id`, `char_title_id`, `spell_item_enchantment_id`, "
              "`warband_scene_id`, `display_file_data_id`, `display_model_scene_id`, `display_name`, `display_tooltip`, "
              "`ordering`, `active`) VALUES\n")
    rows = []
    for index, d in enumerate(catalog["deliverables"]):
        if d["type"] == MOUNT_TYPE and d["mount_spell"]:
            delivery_type, entry, active = 2, d["mount_spell"], 1
        elif d["type"] == TOY_TYPE and d["item"]:
            delivery_type, entry, active = 3, d["item"], 1
        elif d["type"] in ITEM_TYPES and d["item"]:
            delivery_type, entry, active = 0, d["item"], 1
        else:
            delivery_type, entry, active = 0, 0, 0
        display = d["display"] or {}
        rows.append(
            f"({d['id']}, {product_by_deliverable.get(d['id'], 0)}, {d['type']}, {delivery_type}, {entry}, "
            f"{max(d['quantity'], 1)}, {d['flags']}, {sql(d['name'])}, {d['item']}, {d['mount_spell']}, {d['pet_creature']}, "
            f"{d['boost']}, {d['appearance']}, {d['transmog_set']}, {d['title']}, {d['enchant']}, 0, {sql(display.get('file'))}, "
            f"{sql(display.get('scene'))}, {sql(display.get('name1', ''))}, {sql(display.get('tooltip', ''))}, {index}, {active})")
    out.write(",\n".join(rows) + ";\n\n")

    bundles = [(p["id"], b, i) for p in catalog["products"] for i, b in enumerate(p["bundled"])]
    out.write(f"DELETE FROM `retail_battlepay_product_bundle` WHERE `product_id` IN ({product_ids});\n")
    if bundles:
        out.write("INSERT INTO `retail_battlepay_product_bundle` (`product_id`, `bundled_product_id`, `ordering`) VALUES\n")
        out.write(",\n".join(f"({p}, {b}, {i})" for p, b, i in bundles) + ";\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=pathlib.Path, help="official PKT 3.1 capture")
    parser.add_argument("output", type=pathlib.Path, help="output SQL file")
    args = parser.parse_args()
    try:
        opcode = server_opcode("SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE")
        payloads = [payload for direction, op, payload in records(args.input) if direction == b"SMSG" and op == opcode]
        if not payloads:
            raise ValueError("the capture has no product list response")
        build = struct.unpack_from("<I", args.input.read_bytes(), 6)[0]
        catalog = read_catalog(max(payloads, key=len))
        with args.output.open("w", encoding="utf-8", newline="\n") as out:
            write_sql(catalog, f"build {build}", out)
        print(f"{len(catalog['groups'])} groups, {len(catalog['products'])} products, {len(catalog['deliverables'])} deliverables")
    except (OSError, ValueError, struct.error) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
