#!/usr/bin/env python3
"""Check P3D v5 GPS SID/STAR import and business-direction order.

The script expects a Little Navmap SQLite database compiled from P3D v5 stock
scenery. It inspects the stored leg order by transition_leg_id and validates
that SID and STAR endpoints match forward flying direction, not reversed order.
"""

from __future__ import annotations

import argparse
import os
import sqlite3
import sys
from pathlib import Path


SPECIAL_WHERE = (
    "a.type = 'GPS' and a.has_gps_overlay = 1 and a.suffix in ('A', 'D')"
)

# These have no fix by design and describe the start of a departure from the
# runway environment: heading/course to altitude, intercept, radial, etc.
NO_FIX_DEPARTURE_TYPES = ("CA", "CD", "CI", "CR", "VA", "VD", "VI", "VM", "VR")

# A runway transition can omit the runway fix and begin with the first named
# leg after the implicit runway start.
SID_FIRST_NAMED_TYPES = ("CF", "DF", "FA", "FC", "FD", "FM", "IF")

# The far end of a SID should be a route continuation/end leg, not a departure
# start leg. VM is included because it is a manual-termination route end.
SID_ROUTE_END_TYPES = ("AF", "CF", "DF", "FM", "HA", "IF", "TF", "VM")

# A STAR transition starts at its entry point. P3D v5 uses these at the first
# stored leg for the stock GPS STAR records checked here.
STAR_ENTRY_TYPES = ("FC", "FD", "IF")


def placeholders(values: tuple[str, ...]) -> str:
    return ",".join("?" for _ in values)


def resolve_db(explicit: Path | None) -> Path:
    if explicit is not None:
        return explicit

    env_db = os.environ.get("LNM_P3DV5_DB")
    if env_db:
        return Path(env_db)

    cwd = Path.cwd()
    candidates = (
        cwd / "../test-profile-p3dv5-cli/little_navmap_p3dv5.sqlite",
        cwd / "test-profile-p3dv5-cli/little_navmap_p3dv5.sqlite",
    )
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def query_one(con: sqlite3.Connection, sql: str, params: tuple = ()) -> sqlite3.Row:
    row = con.execute(sql, params).fetchone()
    if row is None:
        raise RuntimeError(f"query returned no rows: {sql}")
    return row


def print_rows(title: str, rows: list[sqlite3.Row]) -> None:
    print(title)
    if not rows:
        print("  none")
        return
    for row in rows:
        print("  " + ", ".join(f"{key}={row[key]}" for key in row.keys()))


def print_distribution(title: str, rows: list[sqlite3.Row]) -> None:
    parts = [f"{row['type']}={row['count']}" for row in rows]
    print(f"{title}: " + (", ".join(parts) if parts else "none"))


def print_patterns(title: str, forward: list[str], anti: list[str]) -> None:
    print(title)
    print("  Forward patterns:")
    for item in forward:
        print(f"    - {item}")
    print("  Anti-patterns:")
    for item in anti:
        print(f"    - {item}")


def print_pattern_counts(title: str, rows: list[tuple[str, int, str]]) -> bool:
    print(title)
    ok = True
    for label, count, kind in rows:
        print(f"  {kind:<12} {label}: {count}")
        if kind == "ANTI" and count:
            ok = False
    return ok


def storage_mode_check(con: sqlite3.Connection) -> bool:
    rows = list(
        con.execute(
            """
            with per_approach as (
              select a.approach_id,
                     a.type,
                     a.suffix,
                     a.has_gps_overlay,
                     count(distinct al.approach_leg_id) as approach_legs,
                     count(distinct t.transition_id) as transitions,
                     count(tl.transition_leg_id) as transition_legs
              from approach a
              left join approach_leg al on al.approach_id = a.approach_id
              left join transition t on t.approach_id = a.approach_id
              left join transition_leg tl on tl.transition_id = t.transition_id
              group by a.approach_id
            )
            select case
                     when type = 'GPS' and has_gps_overlay = 1 and suffix = 'D' then 'SID'
                     when type = 'GPS' and has_gps_overlay = 1 and suffix = 'A' then 'STAR'
                     else 'APPR'
                   end as category,
                   count(*) as approaches,
                   sum(approach_legs) as approach_legs,
                   sum(transitions) as transitions,
                   sum(transition_legs) as transition_legs,
                   sum(case
                         when type = 'GPS' and has_gps_overlay = 1 and suffix in ('A', 'D')
                              and (approach_legs <> 0 or transition_legs = 0)
                         then 1
                         when not (type = 'GPS' and has_gps_overlay = 1 and suffix in ('A', 'D'))
                              and approach_legs = 0
                         then 1
                         else 0
                       end) as violations
            from per_approach
            group by category
            order by category
            """
        )
    )
    print_patterns(
        "Storage mode pattern definitions:",
        [
            "SID/STAR records are transition-only: no approach_leg rows, at least one transition_leg row.",
            "APPR records keep normal main approach legs.",
        ],
        [
            "SID/STAR has main approach legs or has no transition legs.",
            "APPR has no main approach legs.",
        ],
    )
    print("Storage mode pattern counts:")
    ok = True
    for row in rows:
        print(
            f"  FORWARD      {row['category']}: "
            f"approaches={row['approaches']}, "
            f"approach_legs={row['approach_legs']}, "
            f"transitions={row['transitions']}, "
            f"transition_legs={row['transition_legs']}"
        )
        print(f"  ANTI         {row['category']} storage violations: {row['violations']}")
        if row["violations"]:
            ok = False
    return ok


def sid_direction_check(con: sqlite3.Connection) -> bool:
    sql = f"""
        with first_last as (
          select t.transition_id,
                 t.fix_ident as transition_name,
                 min(tl.transition_leg_id) as first_id,
                 max(tl.transition_leg_id) as last_id
          from approach a
          join transition t on t.approach_id = a.approach_id
          join transition_leg tl on tl.transition_id = t.transition_id
          where a.type = 'GPS'
            and a.has_gps_overlay = 1
            and a.suffix = 'D'
            and {{transition_filter}}
          group by t.transition_id
        ),
        classified as (
          select first.type as first_type,
                 first.fix_type as first_fix_type,
                 first.fix_ident as first_fix_ident,
                 last.type as last_type
          from first_last fl
          join transition_leg first on first.transition_leg_id = fl.first_id
          join transition_leg last on last.transition_leg_id = fl.last_id
        )
        select count(*) as transitions,
               sum(case when first_fix_type = 'R' then 1 else 0 end) as first_explicit_runway,
               sum(case
                     when first_fix_type is null
                          and first_type in ({placeholders(NO_FIX_DEPARTURE_TYPES)})
                     then 1 else 0
                   end) as first_no_fix_departure,
               sum(case
                     when not (first_fix_type = 'R')
                          and not (first_fix_type is null
                                   and first_type in ({placeholders(NO_FIX_DEPARTURE_TYPES)}))
                          and first_type in ({placeholders(SID_FIRST_NAMED_TYPES)})
                          and coalesce(first_fix_ident, '') <> ''
                     then 1 else 0
                   end) as first_implicit_runway_to_named_fix,
               sum(case
                     when not (
                       first_fix_type = 'R'
                       or (first_fix_type is null
                           and first_type in ({placeholders(NO_FIX_DEPARTURE_TYPES)}))
                       or (first_type in ({placeholders(SID_FIRST_NAMED_TYPES)})
                           and coalesce(first_fix_ident, '') <> '')
                     )
                     then 1 else 0
                   end) as start_violations,
               sum(case
                     when last_type in ({placeholders(SID_ROUTE_END_TYPES)})
                     then 1 else 0
                   end) as last_route_end,
               sum(case
                     when last_type not in ({placeholders(SID_ROUTE_END_TYPES)})
                     then 1 else 0
                   end) as end_violations
        from classified
    """
    params = (
        NO_FIX_DEPARTURE_TYPES
        + NO_FIX_DEPARTURE_TYPES
        + SID_FIRST_NAMED_TYPES
        + NO_FIX_DEPARTURE_TYPES
        + SID_FIRST_NAMED_TYPES
        + SID_ROUTE_END_TYPES
        + SID_ROUTE_END_TYPES
    )

    runway = query_one(con, sql.format(transition_filter="t.fix_ident like 'RW%'"), params)
    enroute = query_one(con, sql.format(transition_filter="t.fix_ident not like 'RW%'"), params)

    print_patterns(
        "SID runway-transition pattern definitions:",
        [
            "First leg is an explicit runway fix.",
            "First leg is a no-fix departure leg such as CA/VA/CD/VI/VM/VD/CI/CR/VR.",
            "First leg is a named fix after an implicit runway start declared by the RW transition.",
            "Last leg is a route/end leg.",
        ],
        [
            "First leg is not one of the departure-start patterns.",
            "Last leg is not a route/end leg.",
        ],
    )
    runway_ok = print_pattern_counts(
        "SID runway-transition pattern counts:",
        [
            ("total runway transitions", runway["transitions"], "INFO"),
            ("first leg is explicit runway", runway["first_explicit_runway"], "FORWARD"),
            ("first leg is no-fix departure leg", runway["first_no_fix_departure"], "FORWARD"),
            (
                "first leg is named fix after implicit runway",
                runway["first_implicit_runway_to_named_fix"],
                "FORWARD",
            ),
            ("first leg is not a departure-start pattern", runway["start_violations"], "ANTI"),
            ("last leg is route/end leg", runway["last_route_end"], "FORWARD"),
            ("last leg is not route/end leg", runway["end_violations"], "ANTI"),
        ],
    )

    print_patterns(
        "SID enroute-transition pattern definitions:",
        [
            "First leg is a named transition start.",
            "Last leg is a route/end leg.",
        ],
        [
            "First leg is not a named transition start.",
            "Last leg is not a route/end leg.",
        ],
    )
    enroute_ok = print_pattern_counts(
        "SID enroute-transition pattern counts:",
        [
            ("total enroute transitions", enroute["transitions"], "INFO"),
            (
                "first leg is named transition start",
                enroute["first_implicit_runway_to_named_fix"],
                "FORWARD",
            ),
            ("first leg is not a named transition start", enroute["start_violations"], "ANTI"),
            ("last leg is route/end leg", enroute["last_route_end"], "FORWARD"),
            ("last leg is not route/end leg", enroute["end_violations"], "ANTI"),
        ],
    )

    no_fix_rows = list(
        con.execute(
            f"""
            with first_last as (
              select t.transition_id, min(tl.transition_leg_id) as first_id
              from approach a
              join transition t on t.approach_id = a.approach_id
              join transition_leg tl on tl.transition_id = t.transition_id
              where a.type = 'GPS'
                and a.has_gps_overlay = 1
                and a.suffix = 'D'
                and t.fix_ident like 'RW%'
              group by t.transition_id
            )
            select first.type as type, count(*) as count
            from first_last fl
            join transition_leg first on first.transition_leg_id = fl.first_id
            where first.fix_type is null
              and first.type in ({placeholders(NO_FIX_DEPARTURE_TYPES)})
            group by first.type
            order by count desc
            """,
            NO_FIX_DEPARTURE_TYPES,
        )
    )
    print_distribution("SID runway-transition no-fix first legs", no_fix_rows)

    return runway_ok and enroute_ok


def star_direction_check(con: sqlite3.Connection) -> bool:
    entry_types = STAR_ENTRY_TYPES
    row = query_one(
        con,
        f"""
        with first_last as (
          select t.transition_id,
                 t.fix_ident as transition_name,
                 min(tl.transition_leg_id) as first_id,
                 max(tl.transition_leg_id) as last_id
          from approach a
          join transition t on t.approach_id = a.approach_id
          join transition_leg tl on tl.transition_id = t.transition_id
          where a.type = 'GPS'
            and a.has_gps_overlay = 1
            and a.suffix = 'A'
          group by t.transition_id
        ),
        classified as (
          select fl.transition_name,
                 first.type as first_type,
                 first.fix_ident as first_fix_ident,
                 last.type as last_type,
                 last.fix_ident as last_fix_ident
          from first_last fl
          join transition_leg first on first.transition_leg_id = fl.first_id
          join transition_leg last on last.transition_leg_id = fl.last_id
        )
        select count(*) as transitions,
               sum(case
                     when first_type in ({placeholders(entry_types)})
                     then 1 else 0
                   end) as first_entry_leg,
               sum(case
                     when first_fix_ident = transition_name
                          and coalesce(transition_name, '') <> ''
                     then 1 else 0
                   end) as first_matches_transition_name,
               sum(case
                     when first_type not in ({placeholders(entry_types)})
                          or first_fix_ident <> transition_name
                     then 1 else 0
                   end) as start_violations
        from classified
        """,
        entry_types + entry_types,
    )
    print_patterns(
        "STAR transition pattern definitions:",
        [
            "First leg is a STAR entry leg.",
            "First fix is the transition name, i.e. the transition starts at its published entry point.",
        ],
        [
            "First leg is not a STAR entry leg.",
            "First fix is not the transition name.",
        ],
    )
    ok = print_pattern_counts(
        "STAR transition pattern counts:",
        [
            ("total STAR transitions", row["transitions"], "INFO"),
            ("first leg is entry leg", row["first_entry_leg"], "FORWARD"),
            (
                "first fix matches transition name",
                row["first_matches_transition_name"],
                "FORWARD",
            ),
            ("first leg is not STAR entry pattern", row["start_violations"], "ANTI"),
        ],
    )

    entry_rows = list(
        con.execute(
            f"""
            with first_last as (
              select t.transition_id, min(tl.transition_leg_id) as first_id
              from approach a
              join transition t on t.approach_id = a.approach_id
              join transition_leg tl on tl.transition_id = t.transition_id
              where a.type = 'GPS'
                and a.has_gps_overlay = 1
                and a.suffix = 'A'
              group by t.transition_id
            )
            select first.type as type, count(*) as count
            from first_last fl
            join transition_leg first on first.transition_leg_id = fl.first_id
            group by first.type
            order by count desc
            """
        )
    )
    print_distribution("STAR first-leg types", entry_rows)

    return ok


def approach_reference_check(con: sqlite3.Connection) -> bool:
    row = query_one(
        con,
        """
        with first_last as (
          select a.approach_id,
                 min(al.approach_leg_id) as first_id
          from approach a
          join approach_leg al on al.approach_id = a.approach_id
          where not (a.type = 'GPS' and a.has_gps_overlay = 1 and a.suffix in ('A', 'D'))
            and al.is_missed = 0
          group by a.approach_id
        )
        select count(*) as approaches,
               sum(case when first.type = 'IF' then 1 else 0 end) as first_initial_fix,
               sum(case when first.type <> 'IF' then 1 else 0 end) as violations
        from first_last fl
        join approach_leg first on first.approach_leg_id = fl.first_id
        """,
    )
    print_patterns(
        "APPR reference pattern definitions:",
        ["First main approach leg is an initial fix."],
        ["First main approach leg is not an initial fix."],
    )
    return print_pattern_counts(
        "APPR reference pattern counts:",
        [
            ("total approaches", row["approaches"], "INFO"),
            ("first leg is initial fix", row["first_initial_fix"], "FORWARD"),
            ("first leg is not initial fix", row["violations"], "ANTI"),
        ],
    )


def print_examples(con: sqlite3.Connection) -> None:
    examples = (
        ("EDDF", "D", "RW18"),
        ("EDDF", "A", "EMPAX"),
        ("ZBAA", "D", "RW36L"),
        ("ZBAA", "A", "BOBAK"),
    )
    print("Forward-order examples:")
    for airport, suffix, transition_name in examples:
        row = con.execute(
            """
            select ar.ident as airport,
                   a.suffix,
                   t.transition_id,
                   t.fix_ident as transition_name,
                   count(tl.transition_leg_id) as leg_count
            from airport ar
            join approach a on a.airport_id = ar.airport_id
            join transition t on t.approach_id = a.approach_id
            join transition_leg tl on tl.transition_id = t.transition_id
            where ar.ident = ?
              and a.suffix = ?
              and a.type = 'GPS'
              and a.has_gps_overlay = 1
              and t.fix_ident = ?
            group by ar.ident, a.approach_id, t.transition_id
            order by a.approach_id, t.transition_id
            limit 1
            """,
            (airport, suffix, transition_name),
        ).fetchone()
        if row is None:
            print(f"  {airport} {suffix} {transition_name}: not found")
            continue

        legs = list(
            con.execute(
                """
                select type, fix_type, fix_ident
                from transition_leg
                where transition_id = ?
                order by transition_leg_id
                """,
                (row["transition_id"],),
            )
        )

        def leg_text(leg: sqlite3.Row) -> str:
            return " ".join(
                part for part in (leg["type"], leg["fix_type"], leg["fix_ident"]) if part
            )

        if len(legs) <= 6:
            sequence = " -> ".join(leg_text(leg) for leg in legs)
        else:
            head = " -> ".join(leg_text(leg) for leg in legs[:4])
            tail = " -> ".join(leg_text(leg) for leg in legs[-2:])
            sequence = f"{head} -> ... -> {tail}"

        kind = "SID" if suffix == "D" else "STAR"
        print(
            f"  {airport} {kind} {transition_name}: "
            f"{row['leg_count']} legs: {sequence}"
        )


def run(db_path: Path) -> bool:
    con = sqlite3.connect(db_path)
    con.row_factory = sqlite3.Row

    metadata = query_one(
        con,
        "select data_source, has_sid_star, db_version_major, db_version_minor from metadata",
    )
    print(
        "Metadata: "
        f"data_source={metadata['data_source']}, "
        f"has_sid_star={metadata['has_sid_star']}, "
        f"db_version={metadata['db_version_major']}.{metadata['db_version_minor']}"
    )

    ok = True
    ok = storage_mode_check(con) and ok
    ok = sid_direction_check(con) and ok
    ok = star_direction_check(con) and ok
    ok = approach_reference_check(con) and ok
    print_examples(con)

    con.close()
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--db",
        type=Path,
        help=(
            "Compiled Little Navmap P3D v5 SQLite database. Defaults to "
            "$LNM_P3DV5_DB or ../test-profile-p3dv5-cli/little_navmap_p3dv5.sqlite."
        ),
    )
    args = parser.parse_args()

    db_path = resolve_db(args.db)
    if not db_path.exists():
        print(f"FAIL database does not exist: {db_path}", file=sys.stderr)
        print("Provide --db or set LNM_P3DV5_DB.", file=sys.stderr)
        return 2

    print(f"Database: {db_path}")
    ok = run(db_path)
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
