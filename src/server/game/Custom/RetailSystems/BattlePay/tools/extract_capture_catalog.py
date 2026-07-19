#!/usr/bin/env python3
"""Extract BattlePay and CatalogShop payloads from a TrinityCore PKT 3.1 capture."""

from __future__ import annotations

import argparse
import collections
import pathlib
import struct
import sys


CMSG_BATTLE_PAY_GET_PRODUCT_LIST = 0x4000E9
CMSG_BATTLE_PAY_OPEN_CHECKOUT = 0x40013A
CMSG_CATALOG_SHOP_LICENSE_GAME_DATA_REQUEST = 0x4000FD
CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE = 0x400115
CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY = 0x400118
CMSG_DB_QUERY_BULK = 0x400010
CMSG_GET_ALL_LICENSED_DECOR_QUANTITIES = 0x290034
CMSG_GET_DECOR_REFUND_LIST = 0x290031
CMSG_HOTFIX_REQUEST = 0x400011
CMSG_BATTLENET_REQUEST = 0x400124
SMSG_AUTH_RESPONSE = 0x420001
SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED = 0x420224
SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE = 0x42021C
SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE = 0x42021A
SMSG_CATALOG_SHOP_LICENSE_DATA = 0x4202BF
SMSG_CATALOG_SHOP_OBTAIN_LICENSE = 0x42036C
SMSG_LAST_CATALOG_FETCH_RESPONSE = 0x42037E
SMSG_SYNC_WOW_ENTITLEMENTS = 0x4202FC
SMSG_COMMERCE_TOKEN_GET_MARKET_PRICE_RESPONSE = 0x42027B
SMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY_RESPONSE = 0x42027F
SMSG_DB_REPLY = 0x460000
SMSG_GET_ALL_LICENSED_DECOR_QUANTITIES_RESPONSE = 0x42037A
SMSG_GET_DECOR_REFUND_LIST_RESPONSE = 0x420375
SMSG_GENERATE_SSO_TOKEN_RESPONSE = 0x4202C5
SMSG_AVAILABLE_HOTFIXES = 0x460001
SMSG_HOTFIX_CONNECT = 0x460003
SMSG_BATTLENET_RESPONSE = 0x4202AD

RESOURCES_SERVICE_HASH = 0xECBE75BA


def read_u32(data: bytes, offset: int) -> tuple[int, int]:
    if offset + 4 > len(data):
        raise ValueError("unexpected end of file")
    return struct.unpack_from("<I", data, offset)[0], offset + 4


def read_packets(path: pathlib.Path) -> tuple[int, list[tuple[bytes, int, bytes]], list[int]]:
    data = path.read_bytes()
    if data[:3] != b"PKT":
        raise ValueError("not a PKT capture")

    offset = 3
    version = struct.unpack_from("<H", data, offset)[0]
    offset += 2
    if version != 0x0301:
        raise ValueError(f"unsupported PKT version 0x{version:04X}; expected 3.1")

    offset += 1  # sniffer ID
    build, offset = read_u32(data, offset)
    offset += 4 + 40 + 4 + 4  # locale, session key, start time, start tick
    additional_length, offset = read_u32(data, offset)
    offset += additional_length

    packets: list[tuple[bytes, int, bytes]] = []
    packet_ticks: list[int] = []
    while offset < len(data):
        if offset + 20 > len(data):
            raise ValueError("truncated packet header")

        direction = data[offset:offset + 4]
        offset += 4
        _connection, tick, additional_size, packet_length = struct.unpack_from("<iIII", data, offset)
        offset += 16 + additional_size
        if packet_length < 4:
            raise ValueError("invalid packet length")

        opcode, offset = read_u32(data, offset)
        payload_size = packet_length - 4
        if offset + payload_size > len(data):
            raise ValueError("truncated packet payload")
        payload = data[offset:offset + payload_size]
        offset += payload_size
        packets.append((direction, opcode, payload))
        packet_ticks.append(tick)

    return build, packets, packet_ticks


def packet_delay(packet_ticks: list[int], request_index: int, response_index: int) -> int:
    delay = (packet_ticks[response_index] - packet_ticks[request_index]) & 0xFFFFFFFF
    if delay > 60_000:
        raise ValueError(f"invalid capture delay {delay} ms")
    return delay


def parse_request(payload: bytes) -> list[int]:
    if len(payload) < 4:
        raise ValueError("truncated CatalogShop request")
    count = struct.unpack_from("<I", payload)[0]
    if count > 4096 or len(payload) != 4 + count * 4:
        raise ValueError(f"invalid CatalogShop request count {count}")
    return list(struct.unpack_from(f"<{count}I", payload, 4)) if count else []


def parse_db_query(payload: bytes) -> tuple[int, list[int]]:
    if len(payload) < 6:
        raise ValueError("truncated DB query")

    table_hash = struct.unpack_from("<I", payload)[0]
    count = (payload[4] << 5) | (payload[5] >> 3)
    if count > 4096 or len(payload) != 6 + count * 4:
        raise ValueError(f"invalid DB query count {count}")

    record_ids = list(struct.unpack_from(f"<{count}I", payload, 6)) if count else []
    return table_hash, record_ids


def parse_db_reply(payload: bytes) -> tuple[int, int, int, bytes]:
    if len(payload) < 17:
        raise ValueError("truncated DB reply")

    table_hash, record_id = struct.unpack_from("<II", payload)
    status = payload[12] >> 5
    data_size = struct.unpack_from("<I", payload, 13)[0]
    if status > 4 or len(payload) != 17 + data_size:
        raise ValueError("invalid DB reply")

    return table_hash, record_id, status, payload[17:]


def extract_db_query_routes(packets: list[tuple[bytes, int, bytes]], first_packet: int,
                            last_packet: int, packet_ticks: list[int]) -> list[tuple[int, int, int,
                                                                                  list[tuple[int, int, bytes]]]]:
    routes: list[tuple[int, int, int, list[tuple[int, int, bytes]]]] = []
    for request_index in range(first_packet, last_packet):
        direction, opcode, payload = packets[request_index]
        if direction != b"CMSG" or opcode != CMSG_DB_QUERY_BULK:
            continue

        table_hash, record_ids = parse_db_query(payload)
        requested_records = set(record_ids)
        replies: dict[int, tuple[int, bytes]] = {}
        first_response_index = -1
        for response_index, (response_direction, response_opcode, response_payload) in enumerate(
                packets[request_index + 1:last_packet], request_index + 1):
            if response_direction != b"SMSG" or response_opcode != SMSG_DB_REPLY:
                continue

            response_table_hash, record_id, status, data = parse_db_reply(response_payload)
            if response_table_hash != table_hash or record_id not in requested_records or record_id in replies:
                continue

            replies[record_id] = (status, data)
            if first_response_index < 0:
                first_response_index = response_index
            if len(replies) == len(record_ids):
                break

        if len(replies) != len(record_ids):
            raise ValueError(
                f"DB query 0x{table_hash:08X} has {len(record_ids)} requests but {len(replies)} replies"
            )

        records = [(record_id, *replies[record_id]) for record_id in record_ids]
        status_counts = collections.Counter(status for _record_id, status, data in records if not data)
        default_status = 0xFFFFFFFF
        if status_counts:
            candidate_status, candidate_count = status_counts.most_common(1)[0]
            if candidate_status == 4 and candidate_count * 2 > len(records):
                # NotPublic is a table-level visibility result. Use it for records requested by
                # another account/client cache but absent from this particular retail capture.
                default_status = candidate_status

        routes.append((table_hash, default_status,
                       packet_delay(packet_ticks, request_index, first_response_index), records))

    return routes


def extract(path: pathlib.Path) -> tuple[int, int, int, bytes, int,
                                             list[tuple[int, bytes, int]],
                                             list[tuple[int, int, bytes, int]],
                                             bytes, int,
                                             list[tuple[int, int, int, list[tuple[int, int, bytes]]]],
                                             int, bytes, bytes, int,
                                             list[tuple[int, int, int]],
                                             list[tuple[int, bytes, int]],
                                             list[tuple[list[int], bytes, int]]]:
    build, packets, packet_ticks = read_packets(path)
    product_payloads: list[bytes] = []
    product_request_index = -1
    product_response_index = -1
    license_ids: list[int] = []
    catalog_versions: list[int] = []
    pending_requests: collections.deque[tuple[list[int], int]] = collections.deque()
    routes: list[tuple[list[int], bytes, int]] = []
    pending_gates: collections.deque[tuple[int, int]] = collections.deque()
    gate_routes: list[tuple[int, int, bytes, int]] = []
    gate_responses = {
        CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE: SMSG_COMMERCE_TOKEN_GET_MARKET_PRICE_RESPONSE,
        CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY: SMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY_RESPONSE,
        CMSG_GET_ALL_LICENSED_DECOR_QUANTITIES: SMSG_GET_ALL_LICENSED_DECOR_QUANTITIES_RESPONSE,
        CMSG_GET_DECOR_REFUND_LIST: SMSG_GET_DECOR_REFUND_LIST_RESPONSE,
    }

    for packet_index, (direction, opcode, payload) in enumerate(packets):
        if direction == b"CMSG" and opcode == CMSG_BATTLE_PAY_GET_PRODUCT_LIST:
            product_request_index = packet_index
        if direction == b"SMSG" and opcode == SMSG_BATTLE_PAY_GET_PRODUCT_LIST_RESPONSE:
            product_payloads.append(payload)
            product_response_index = packet_index
        elif direction == b"SMSG" and opcode == SMSG_CATALOG_SHOP_OBTAIN_LICENSE:
            if len(payload) != 4:
                raise ValueError("invalid CatalogShop license packet")
            license_ids.append(struct.unpack_from("<I", payload)[0])
        elif direction == b"SMSG" and opcode == SMSG_LAST_CATALOG_FETCH_RESPONSE:
            if len(payload) != 8:
                raise ValueError("invalid last-catalog-fetch response")
            catalog_versions.append(struct.unpack_from("<Q", payload)[0])
        elif direction == b"CMSG" and opcode == CMSG_CATALOG_SHOP_LICENSE_GAME_DATA_REQUEST:
            pending_requests.append((parse_request(payload), packet_index))
        elif direction == b"SMSG" and opcode == SMSG_CATALOG_SHOP_LICENSE_DATA:
            if not pending_requests:
                raise ValueError("CatalogShop response has no preceding request")
            request_ids, request_index = pending_requests.popleft()
            routes.append((request_ids, payload, packet_delay(packet_ticks, request_index, packet_index)))
        elif direction == b"CMSG" and opcode in gate_responses:
            pending_gates.append((opcode, packet_index))
        elif (direction == b"SMSG" and pending_gates and
              opcode == gate_responses[pending_gates[0][0]]):
            request_opcode, request_index = pending_gates.popleft()
            gate_routes.append((request_opcode, opcode, payload,
                                packet_delay(packet_ticks, request_index, packet_index)))

    if not product_payloads:
        raise ValueError("BattlePay product-list response was not found")
    product_payload = max(product_payloads, key=len)
    if len(product_payload) < 24:
        raise ValueError("BattlePay product-list response is truncated")

    result, currency, products, deliverables, groups, shops = struct.unpack_from("<6I", product_payload)
    if result != 0 or not products or not shops:
        raise ValueError("capture does not contain a usable BattlePay catalog")
    if not license_ids or len(set(license_ids)) != 1:
        raise ValueError("capture has no single stable CatalogShop license ID")
    if not catalog_versions or len(set(catalog_versions)) != 1:
        raise ValueError("capture has no single stable catalog version")
    if pending_requests:
        raise ValueError("capture has CatalogShop requests without responses")
    if not routes:
        raise ValueError("capture has no CatalogShop request/response routes")
    if pending_gates or [request for request, _response, _payload, _delay in gate_routes] != [
        CMSG_COMMERCE_TOKEN_GET_MARKET_PRICE,
        CMSG_GET_DECOR_REFUND_LIST,
        CMSG_GET_ALL_LICENSED_DECOR_QUANTITIES,
        CMSG_CONSUMABLE_TOKEN_CAN_VETERAN_BUY,
    ]:
        raise ValueError("capture does not contain the expected store gate responses")
    if product_request_index < 0 or product_response_index <= product_request_index:
        raise ValueError("capture has no complete BattlePay product bootstrap")

    first_catalog_request_index = next(
        (index for index, (direction, opcode, _payload) in enumerate(packets[product_response_index + 1:],
                                                                    product_response_index + 1)
         if direction == b"CMSG" and opcode == CMSG_CATALOG_SHOP_LICENSE_GAME_DATA_REQUEST),
        -1,
    )
    if first_catalog_request_index < 0:
        raise ValueError("capture has no CatalogShop request after the BattlePay product list")

    db_query_routes = extract_db_query_routes(
        packets, product_response_index + 1, first_catalog_request_index, packet_ticks
    )
    if not db_query_routes:
        raise ValueError("capture has no store DB query route")

    open_checkout_index = next(
        (index for index, (direction, opcode, _payload) in enumerate(packets[product_response_index + 1:],
                                                                    product_response_index + 1)
         if direction == b"CMSG" and opcode == CMSG_BATTLE_PAY_OPEN_CHECKOUT),
        -1,
    )
    if open_checkout_index < 0 or open_checkout_index >= first_catalog_request_index:
        raise ValueError("capture has no BattlePay open-checkout request")

    open_checkout_response_index = next(
        (index for index, (direction, opcode, _payload) in enumerate(
            packets[open_checkout_index + 1:first_catalog_request_index], open_checkout_index + 1)
         if direction == b"SMSG" and opcode == SMSG_GENERATE_SSO_TOKEN_RESPONSE), -1)
    open_checkout_response = (packets[open_checkout_response_index][2]
                              if open_checkout_response_index >= 0 else b"")
    if len(open_checkout_response) < 25:
        raise ValueError("capture has no valid SSO token response")

    issued_at, expires_at = struct.unpack_from("<QQ", open_checkout_response, 8)
    token_length = open_checkout_response[24] >> 1
    if issued_at >= expires_at or expires_at - issued_at > 24 * 60 * 60 or 25 + token_length != len(open_checkout_response):
        raise ValueError("capture has an invalid SSO token response")

    pre_product_opcodes = {
        SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED,
        SMSG_SYNC_WOW_ENTITLEMENTS,
        SMSG_CATALOG_SHOP_OBTAIN_LICENSE,
    }
    pre_product_packets = [
        (opcode, payload, packet_delay(packet_ticks, product_request_index, index))
        for index, (direction, opcode, payload) in enumerate(
            packets[product_request_index + 1:product_response_index], product_request_index + 1)
        if direction == b"SMSG" and opcode in pre_product_opcodes
    ]
    if [opcode for opcode, _payload, _delay in pre_product_packets] != [
        SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED,
        SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED,
        SMSG_SYNC_WOW_ENTITLEMENTS,
        SMSG_CATALOG_SHOP_OBTAIN_LICENSE,
    ]:
        raise ValueError("capture does not contain the expected entitlement bootstrap")

    product_response_delay = packet_delay(packet_ticks, product_request_index, product_response_index)
    open_checkout_delay = packet_delay(packet_ticks, open_checkout_index, open_checkout_response_index)

    last_catalog_response_index = next(
        (index for index, (direction, opcode, payload) in enumerate(
            packets[product_request_index + 1:first_catalog_request_index], product_request_index + 1)
         if direction == b"SMSG" and opcode == SMSG_LAST_CATALOG_FETCH_RESPONSE and
         len(payload) == 8 and struct.unpack_from("<Q", payload)[0] == catalog_versions[-1]), -1)
    if last_catalog_response_index < 0:
        raise ValueError("capture has no initial last-catalog-fetch response")
    last_catalog_delay = packet_delay(packet_ticks, product_request_index, last_catalog_response_index)

    available_hotfixes = max(
        (payload for direction, opcode, payload in packets[:first_catalog_request_index]
         if direction == b"SMSG" and opcode == SMSG_AVAILABLE_HOTFIXES), key=len, default=b"")
    if len(available_hotfixes) < 8:
        raise ValueError("capture has no available-hotfixes bootstrap")

    hotfix_request_index = next(
        (index for index, (direction, opcode, _payload) in enumerate(
            packets[product_request_index:first_catalog_request_index], product_request_index)
         if direction == b"CMSG" and opcode == CMSG_HOTFIX_REQUEST), -1)
    hotfix_response_index = next(
        (index for index, (direction, opcode, _payload) in enumerate(
            packets[hotfix_request_index + 1:first_catalog_request_index], hotfix_request_index + 1)
         if direction == b"SMSG" and opcode == SMSG_HOTFIX_CONNECT), -1)
    if hotfix_request_index < 0 or hotfix_response_index < 0:
        raise ValueError("capture has no complete hotfix request/response route")
    hotfix_connect = packets[hotfix_response_index][2]
    hotfix_delay = packet_delay(packet_ticks, hotfix_request_index, hotfix_response_index)

    rpc_delays: dict[tuple[int, int], list[int]] = collections.defaultdict(list)
    pending_rpc: dict[tuple[int, int, int], int] = {}
    for packet_index, (direction, opcode, payload) in enumerate(packets[:first_catalog_request_index]):
        if direction == b"CMSG" and opcode == CMSG_BATTLENET_REQUEST and len(payload) >= 24:
            method_type, _object_id, token, payload_size = struct.unpack_from("<QQII", payload)
            service_hash, method_id = method_type >> 32, method_type & 0xFFFFFFFF
            if payload_size == len(payload) - 24:
                pending_rpc[(service_hash, method_id, token)] = packet_index
        elif direction == b"SMSG" and opcode == SMSG_BATTLENET_RESPONSE and len(payload) >= 28:
            method_type, _object_id, token, payload_size = struct.unpack_from("<QQII", payload, 4)
            service_hash, method_id = method_type >> 32, method_type & 0xFFFFFFFF
            key = (service_hash, method_id, token)
            if key in pending_rpc and payload_size == len(payload) - 28:
                rpc_delays[(service_hash, method_id)].append(
                    packet_delay(packet_ticks, pending_rpc.pop(key), packet_index))

    captured_rpc_delays = [
        (service_hash, method_id, min(delays))
        for (service_hash, method_id), delays in sorted(rpc_delays.items())
    ]
    if [method_id for service_hash, method_id, _delay in captured_rpc_delays
            if service_hash == RESOURCES_SERVICE_HASH] != [1, 2]:
        raise ValueError("capture has no complete ResourcesService timing routes")

    auth_response_index = max(
        (index for index, (direction, opcode, _payload) in enumerate(packets[:product_request_index])
         if direction == b"SMSG" and opcode == SMSG_AUTH_RESPONSE), default=-1)
    if auth_response_index < 0:
        raise ValueError("capture has no auth response before the BattlePay bootstrap")

    login_bootstrap_opcodes = {
        SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE,
        SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED,
        SMSG_CATALOG_SHOP_OBTAIN_LICENSE,
    }
    login_bootstrap_packets = [
        (opcode, payload, packet_delay(packet_ticks, auth_response_index, index))
        for index, (direction, opcode, payload) in enumerate(
            packets[auth_response_index + 1:product_request_index], auth_response_index + 1)
        if direction == b"SMSG" and opcode in login_bootstrap_opcodes
    ]
    if [opcode for opcode, _payload, _delay in login_bootstrap_packets] != [
        SMSG_BATTLE_PAY_GET_DISTRIBUTION_LIST_RESPONSE,
        SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED,
        SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED,
        SMSG_BATTLE_PAY_DELIVERABLE_DELIVERED,
        SMSG_CATALOG_SHOP_OBTAIN_LICENSE,
    ]:
        raise ValueError("capture does not contain the expected BattlePay login bootstrap")

    print(
        f"build={build} license={license_ids[-1]} catalog_version={catalog_versions[-1]} currency={currency} "
        f"products={products} deliverables={deliverables} groups={groups} shops={shops} "
        f"product_bytes={len(product_payload)} bootstrap_packets={len(pre_product_packets)} "
        f"gate_routes={len(gate_routes)} sso_bytes={len(open_checkout_response)} "
        f"db_query_routes={len(db_query_routes)} routes={len(routes)}"
    )
    print(
        f"timing: product={product_response_delay}ms last_catalog={last_catalog_delay}ms "
        f"hotfix={hotfix_delay}ms open_checkout={open_checkout_delay}ms"
    )
    for index, (opcode, payload, delay) in enumerate(pre_product_packets, 1):
        print(f"bootstrap[{index}]: opcode=0x{opcode:06X} bytes={len(payload)} delay={delay}ms")
    for index, (request_opcode, response_opcode, payload, delay) in enumerate(gate_routes, 1):
        print(f"gate[{index}]: request=0x{request_opcode:06X} response=0x{response_opcode:06X} "
              f"bytes={len(payload)} delay={delay}ms")
    for index, (table_hash, default_status, delay, records) in enumerate(db_query_routes, 1):
        statuses = collections.Counter(status for _record_id, status, _data in records)
        default_name = "none" if default_status == 0xFFFFFFFF else str(default_status)
        print(
            f"db_query[{index}]: table=0x{table_hash:08X} records={len(records)} "
            f"statuses={dict(statuses)} default={default_name} delay={delay}ms"
        )
    for service_hash, method_id, delay in captured_rpc_delays:
        print(f"rpc: service=0x{service_hash:08X} method={method_id} delay={delay}ms")
    for index, (opcode, payload, delay) in enumerate(login_bootstrap_packets, 1):
        print(f"login_bootstrap[{index}]: opcode=0x{opcode:06X} bytes={len(payload)} delay={delay}ms")
    for index, (request_ids, response, delay) in enumerate(routes, 1):
        print(f"route[{index}]: request_ids={len(request_ids)} response_bytes={len(response)} delay={delay}ms")

    return (build, license_ids[-1], catalog_versions[-1], product_payload, product_response_delay,
            pre_product_packets, gate_routes, open_checkout_response, open_checkout_delay,
            db_query_routes, last_catalog_delay, available_hotfixes, hotfix_connect, hotfix_delay,
            captured_rpc_delays, login_bootstrap_packets, routes)


def write_capture(output: pathlib.Path, build: int, license_id: int, catalog_version: int, product: bytes,
                  product_response_delay: int,
                  pre_product_packets: list[tuple[int, bytes, int]],
                  gate_routes: list[tuple[int, int, bytes, int]],
                  open_checkout_response: bytes, open_checkout_delay: int,
                  db_query_routes: list[tuple[int, int, int, list[tuple[int, int, bytes]]]],
                  last_catalog_delay: int, available_hotfixes: bytes, hotfix_connect: bytes,
                  hotfix_delay: int, rpc_delays: list[tuple[int, int, int]],
                  login_bootstrap_packets: list[tuple[int, bytes, int]],
                  routes: list[tuple[list[int], bytes, int]]) -> None:
    payload = bytearray(b"TCBP")
    payload += struct.pack("<IIIQ", 9, build, license_id, catalog_version)
    payload += struct.pack("<I", len(product))
    payload += product
    payload += struct.pack("<I", product_response_delay)
    payload += struct.pack("<I", len(pre_product_packets))
    for opcode, packet_payload, delay in pre_product_packets:
        payload += struct.pack("<II", opcode, len(packet_payload))
        payload += packet_payload
        payload += struct.pack("<I", delay)
    payload += struct.pack("<I", len(gate_routes))
    for request_opcode, response_opcode, response_payload, delay in gate_routes:
        payload += struct.pack("<III", request_opcode, response_opcode, len(response_payload))
        payload += response_payload
        payload += struct.pack("<I", delay)
    payload += struct.pack("<I", len(open_checkout_response))
    payload += open_checkout_response
    payload += struct.pack("<I", open_checkout_delay)
    payload += struct.pack("<I", len(db_query_routes))
    for table_hash, default_status, delay, records in db_query_routes:
        payload += struct.pack("<III", table_hash, default_status, len(records))
        for record_id, status, data in records:
            payload += struct.pack("<III", record_id, status, len(data))
            payload += data
        payload += struct.pack("<I", delay)
    payload += struct.pack("<I", last_catalog_delay)
    payload += struct.pack("<I", len(available_hotfixes))
    payload += available_hotfixes
    payload += struct.pack("<I", len(hotfix_connect))
    payload += hotfix_connect
    payload += struct.pack("<I", hotfix_delay)
    payload += struct.pack("<I", len(rpc_delays))
    for service_hash, method_id, delay in rpc_delays:
        payload += struct.pack("<III", service_hash, method_id, delay)
    payload += struct.pack("<I", len(login_bootstrap_packets))
    for opcode, packet_payload, delay in login_bootstrap_packets:
        payload += struct.pack("<II", opcode, len(packet_payload))
        payload += packet_payload
        payload += struct.pack("<I", delay)
    payload += struct.pack("<I", len(routes))
    for request_ids, response, delay in routes:
        payload += struct.pack("<I", len(request_ids))
        if request_ids:
            payload += struct.pack(f"<{len(request_ids)}I", *request_ids)
        payload += struct.pack("<I", len(response))
        payload += response
        payload += struct.pack("<I", delay)

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(payload)
    print(f"wrote {len(payload)} bytes to {output}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=pathlib.Path, help="official PKT 3.1 capture")
    parser.add_argument("output", type=pathlib.Path, help="output .tcbp capture catalog")
    args = parser.parse_args()

    try:
        write_capture(args.output, *extract(args.input))
    except (OSError, ValueError, struct.error) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
