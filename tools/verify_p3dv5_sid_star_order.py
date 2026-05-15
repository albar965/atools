#!/usr/bin/env python3
"""Verify P3D v5 GPS SID/STAR import and transition leg order.

This helper expects a Little Navmap SQLite database compiled from P3D v5
stock scenery after the atools import fix. It checks the transition-only
GPS overlay SID/STAR shape and the database ordering consumed by LNM.
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

NO_FIX_DEPARTURE_TYPES = {
    "CA",  # course to altitude
    "CD",  # course to DME distance
    "CI",  # course to intercept
    "CR",  # course to radial
    "VA",  # heading to altitude
    "VD",  # heading to DME distance
    "VI",  # heading to intercept
    "VM",  # heading to manual termination
    "VR",  # heading to radial
}

SID_FIRST_NAMED_TYPES = {
    "IF",  # initial fix
    "CF",  # course to fix
    "DF",  # direct to fix
    "FA",  # fix to altitude
    "FC",  # fix to distance
    "FD",  # fix to DME distance
    "FM",  # fix to manual termination
}

STAR_ENTRY_TYPES = {
    "IF",  # initial fix
    "FD",  # fix to DME distance
    "FC",  # fix to distance
}


def query_one(con: sqlite3.Connection, sql: str, params: tuple = ()) -> sqlite3.Row:
    row = con.execute(sql, params).fetchone()
    if row is None:
        raise RuntimeError(f"query returned no rows: {sql}")
    return row


def print_rows(title: str, rows: list[sqlite3.Row]) -> None:
    print(title)
    if rows:
        for row in rows:
            print("  " + ", ".join(f"{key}={row[key]}" for key in row.keys()))
    else:
        print("  none")


def csv_placeholders(items: list[str] | tuple[str, ...] | set[str]) -> str:
    return ",".join("?" for _ in items)


def resolve_default_db(explicit: Path | None) -> Path | None:
    if explicit is not None:
        return explicit

    env_db = os.environ.get("LNM_P3DV5_DB")
    if env_db:
        return Path(env_db)

    here = Path.cwd()
    candidates = [
        here / "../test-profile-p3dv5-cli/little_navmap_p3dv5.sqlite",
        here / "test-profile-p3dv5-cli/little_navmap_p3dv5.sqlite",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


def resolve_default_atools(explicit: Path | None) -> Path | None:
    if explicit is not None:
        return explicit
    here = Path.cwd()
    if (here / "src/fs/bgl/ap/transition.cpp").exists():
        return here
    return None


def resolve_default_lnm(explicit: Path | None) -> Path | None:
    if explicit is not None:
        return explicit
    sibling = Path.cwd() / "../littlenavmap"
    if (sibling / "src/query/procedurequery.cpp").exists():
        return sibling
    return None


def source_contains(path: Path, needles: list[str]) -> tuple[bool, str]:
    if not path.exists():
        return False, f"missing {path}"
    text = path.read_text(encoding="utf-8", errors="replace")
    missing = [needle for needle in needles if needle not in text]
    if missing:
        return False, f"{path}: missing {missing!r}"
    return True, str(path)


def run_source_checks(atools_src: Path | None, lnm_src: Path | None) -> bool:
    checks: list[tuple[str, bool, str]] = []

    if atools_src is not None:
        checks.append(
            (
                "BGL transition parser appends legs",
                *source_contains(
                    atools_src / "src/fs/bgl/ap/transition.cpp",
                    ["legs.append(ApproachLeg(stream, subRecType))"],
                ),
            )
        )
        checks.append(
            (
                "DB writer writes transition legs in container order",
                *source_contains(
                    atools_src / "src/fs/db/ap/transitionwriter.cpp",
                    ["write(type->getLegs())"],
                ),
            )
        )
        checks.append(
            (
                "DB writer assigns increasing transition_leg_id",
                *source_contains(
                    atools_src / "src/fs/db/ap/transitionlegwriter.cpp",
                    ["transition_leg_id", "getNextId()"],
                ),
            )
        )

    if lnm_src is not None:
        checks.append(
            (
                "LNM reads transition legs by transition_leg_id",
                *source_contains(
                    lnm_src / "src/query/procedurequery.cpp",
                    ["order by transition_leg_id"],
                ),
            )
        )

    if not checks:
        return True

    ok = True
    print("Source order checks:")
    for name, passed, detail in checks:
        print(f"  {'OK' if passed else 'FAIL'} {name}: {detail}")
        ok = ok and passed
    return ok


def run_db_checks(db_path: Path, sample_airports: list[str]) -> bool:
    con = sqlite3.connect(db_path)
    con.row_factory = sqlite3.Row

    metadata = query_one(
        con,
        "select data_source, has_sid_star, db_version_major, db_version_minor "
        "from metadata",
    )
    print(
        "Metadata: "
        f"data_source={metadata['data_source']}, "
        f"has_sid_star={metadata['has_sid_star']}, "
        f"db_version={metadata['db_version_major']}.{metadata['db_version_minor']}"
    )

    mode_counts = list(
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
                       end) as storage_mode_violations
            from per_approach
            group by category
            order by category
            """
        )
    )
    print_rows(
        "Storage mode by category "
        "(SID/STAR are transition-only; APPR has main approach legs):",
        mode_counts,
    )

    ok = True
    if any(row["storage_mode_violations"] for row in mode_counts):
        ok = False

    counts = list(
        con.execute(
            f"""
            select a.suffix,
                   count(distinct a.approach_id) as approaches,
                   count(distinct t.transition_id) as transitions,
                   count(distinct al.approach_leg_id) as approach_legs,
                   count(tl.transition_leg_id) as transition_legs
            from approach a
            left join approach_leg al on al.approach_id = a.approach_id
            left join transition t on t.approach_id = a.approach_id
            left join transition_leg tl on tl.transition_id = t.transition_id
            where {SPECIAL_WHERE}
            group by a.suffix
            order by a.suffix
            """
        )
    )
    print_rows("P3D v5 GPS overlay SID/STAR counts:", counts)

    by_suffix = {row["suffix"]: row for row in counts}
    for suffix in ("A", "D"):
        row = by_suffix.get(suffix)
        if row is None or row["approaches"] == 0 or row["transition_legs"] == 0:
            print(f"FAIL missing suffix {suffix} transition-only records")
            ok = False
        elif row["approach_legs"] != 0:
            print(f"FAIL suffix {suffix} has main approach legs: {row['approach_legs']}")
            ok = False

    bad_transitions = query_one(
        con,
        f"""
        select count(*) as count
        from (
          select t.transition_id
          from approach a
          join transition t on t.approach_id = a.approach_id
          left join transition_leg tl on tl.transition_id = t.transition_id
          where {SPECIAL_WHERE}
          group by t.transition_id
          having count(tl.transition_leg_id) = 0
        )
        """,
    )["count"]
    print(f"Transitions without legs: {bad_transitions}")
    if bad_transitions:
        ok = False

    non_contiguous = list(
        con.execute(
            f"""
            select suffix, count(*) as transitions
            from (
              select a.suffix, t.transition_id,
                     min(tl.transition_leg_id) as first_leg_id,
                     max(tl.transition_leg_id) as last_leg_id,
                     count(tl.transition_leg_id) as leg_count
              from approach a
              join transition t on t.approach_id = a.approach_id
              join transition_leg tl on tl.transition_id = t.transition_id
              where {SPECIAL_WHERE}
              group by a.suffix, t.transition_id
              having last_leg_id - first_leg_id + 1 <> leg_count
            )
            group by suffix
            order by suffix
            """
        )
    )
    print_rows("Transitions with non-contiguous transition_leg_id blocks:", non_contiguous)
    if non_contiguous:
        ok = False

    appr_forward = query_one(
        con,
        """
        with first_last as (
          select a.approach_id,
                 min(al.approach_leg_id) as first_id,
                 max(al.approach_leg_id) as last_id
          from approach a
          join approach_leg al on al.approach_id = a.approach_id
          where not (a.type = 'GPS' and a.has_gps_overlay = 1 and a.suffix in ('A', 'D'))
            and al.is_missed = 0
          group by a.approach_id
        )
        select count(*) as approaches,
               sum(case when first.type = 'IF' then 1 else 0 end) as first_initial_fix,
               sum(case when first.type <> 'IF' then 1 else 0 end) as first_leg_violations
        from first_last fl
        join approach_leg first on first.approach_leg_id = fl.first_id
        """
    )
    print(
        "APPR forward pattern: "
        f"approaches={appr_forward['approaches']}, "
        f"first initial-fix={appr_forward['first_initial_fix']}, "
        f"violations={appr_forward['first_leg_violations']}"
    )
    if appr_forward["first_leg_violations"]:
        ok = False

    no_fix_departure_types = sorted(NO_FIX_DEPARTURE_TYPES)
    sid_first_named_types = sorted(SID_FIRST_NAMED_TYPES)
    sid_forward = query_one(
        con,
        f"""
        with first_last as (
          select t.transition_id,
                 min(tl.transition_leg_id) as first_id,
                 max(tl.transition_leg_id) as last_id
          from approach a
          join transition t on t.approach_id = a.approach_id
          join transition_leg tl on tl.transition_id = t.transition_id
          where {SPECIAL_WHERE} and a.suffix = 'D'
          group by t.transition_id
        )
        select count(*) as transitions,
               sum(case when first.fix_type = 'R' then 1 else 0 end) as first_explicit_runway,
               sum(case when first.fix_type is null
                             and first.type in ({csv_placeholders(no_fix_departure_types)})
                        then 1 else 0 end) as first_departure_start,
               sum(case when first.type in ({csv_placeholders(sid_first_named_types)})
                             and coalesce(first.fix_ident, '') <> ''
                        then 1 else 0 end) as first_named_start,
               sum(case when not (
                          first.fix_type = 'R'
                          or (first.fix_type is null
                              and first.type in ({csv_placeholders(no_fix_departure_types)}))
                          or (first.type in ({csv_placeholders(sid_first_named_types)})
                              and coalesce(first.fix_ident, '') <> '')
                        )
                        then 1 else 0 end) as first_leg_violations,
               sum(case when last.fix_ident is not null
                             and coalesce(last.fix_type, '') <> 'R'
                        then 1 else 0 end) as last_named_non_runway_fix,
               sum(case when last.fix_type = 'R' then 1 else 0 end) as last_explicit_runway
        from first_last fl
        join transition_leg first on first.transition_leg_id = fl.first_id
        join transition_leg last on last.transition_leg_id = fl.last_id
        """,
        tuple(no_fix_departure_types)
        + tuple(sid_first_named_types)
        + tuple(no_fix_departure_types)
        + tuple(sid_first_named_types),
    )
    print(
        "SID forward pattern: "
        f"transitions={sid_forward['transitions']}, "
        f"first explicit-runway={sid_forward['first_explicit_runway']}, "
        f"first departure-start={sid_forward['first_departure_start']}, "
        f"first named-start={sid_forward['first_named_start']}, "
        f"violations={sid_forward['first_leg_violations']}, "
        f"last named non-runway fix={sid_forward['last_named_non_runway_fix']}, "
        f"last explicit runway={sid_forward['last_explicit_runway']}"
    )
    if sid_forward["first_leg_violations"]:
        ok = False

    star_entry_types = sorted(STAR_ENTRY_TYPES)
    star_forward = query_one(
        con,
        f"""
        with first_last as (
          select t.transition_id,
                 min(tl.transition_leg_id) as first_id,
                 max(tl.transition_leg_id) as last_id
          from approach a
          join transition t on t.approach_id = a.approach_id
          join transition_leg tl on tl.transition_id = t.transition_id
          where {SPECIAL_WHERE} and a.suffix = 'A'
          group by t.transition_id
        )
        select count(*) as transitions,
               sum(case when first.type in ({csv_placeholders(star_entry_types)})
                        then 1 else 0 end) as first_entry_like,
               sum(case when first.type not in ({csv_placeholders(star_entry_types)})
                        then 1 else 0 end) as first_leg_violations,
               sum(case when last.type in ({csv_placeholders(star_entry_types)})
                        then 1 else 0 end) as last_entry_like
        from first_last fl
        join transition_leg first on first.transition_leg_id = fl.first_id
        join transition_leg last on last.transition_leg_id = fl.last_id
        """,
        tuple(star_entry_types) * 3,
    )
    print(
        "STAR forward pattern: "
        f"transitions={star_forward['transitions']}, "
        f"first entry-like={star_forward['first_entry_like']}, "
        f"violations={star_forward['first_leg_violations']}, "
        f"last entry-like={star_forward['last_entry_like']}"
    )
    if star_forward["first_leg_violations"]:
        ok = False

    if sample_airports:
        placeholders = ",".join("?" for _ in sample_airports)
        samples = list(
            con.execute(
                f"""
                select ar.ident,
                       count(distinct case when a.suffix = 'A' then a.approach_id end) as star_approaches,
                       count(distinct case when a.suffix = 'D' then a.approach_id end) as sid_approaches,
                       count(distinct t.transition_id) as transitions,
                       count(tl.transition_leg_id) as transition_legs
                from airport ar
                join approach a on a.airport_id = ar.airport_id
                left join transition t on t.approach_id = a.approach_id
                left join transition_leg tl on tl.transition_id = t.transition_id
                where ar.ident in ({placeholders}) and {SPECIAL_WHERE}
                group by ar.ident
                order by ar.ident
                """,
                tuple(sample_airports),
            )
        )
        print_rows("Sample airports:", samples)

        print("Forward-order examples:")
        examples = [
            ("EDDF", "D", "RW18"),
            ("EDDF", "A", "EMPAX"),
            ("ZBAA", "D", "RW36L"),
            ("ZBAA", "A", "BOBAK"),
        ]
        for airport, suffix, transition_ident in examples:
            print_example(con, airport, suffix, transition_ident)

    con.close()
    return ok


def print_example(
    con: sqlite3.Connection, airport: str, suffix: str, transition_ident: str
) -> None:
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
        (airport, suffix, transition_ident),
    ).fetchone()
    if row is None:
        print(f"  {airport} {suffix} {transition_ident}: not found")
        return

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
        fix = leg["fix_ident"] or ""
        fix_type = leg["fix_type"] or ""
        return f"{leg['type']} {fix_type} {fix}".strip()

    if len(legs) <= 6:
        sequence = " -> ".join(leg_text(leg) for leg in legs)
    else:
        head = " -> ".join(leg_text(leg) for leg in legs[:4])
        tail = " -> ".join(leg_text(leg) for leg in legs[-2:])
        sequence = f"{head} -> ... -> {tail}"

    kind = "SID" if suffix == "D" else "STAR"
    print(
        f"  {airport} {kind} {transition_ident}: "
        f"{row['leg_count']} legs: {sequence}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--db",
        type=Path,
        help=(
            "Compiled Little Navmap SQLite database. Defaults to "
            "$LNM_P3DV5_DB or ../test-profile-p3dv5-cli/little_navmap_p3dv5.sqlite."
        ),
    )
    parser.add_argument("--atools-src", type=Path, help="Optional atools source tree for source order checks")
    parser.add_argument("--lnm-src", type=Path, help="Optional Little Navmap source tree for read order checks")
    parser.add_argument("--sample-airport", action="append", default=["EDDF", "ZBAA"])
    args = parser.parse_args()

    db_path = resolve_default_db(args.db)
    if db_path is None or not db_path.exists():
        print(f"FAIL database does not exist: {db_path}", file=sys.stderr)
        print("Provide --db or set LNM_P3DV5_DB.", file=sys.stderr)
        return 2

    atools_src = resolve_default_atools(args.atools_src)
    lnm_src = resolve_default_lnm(args.lnm_src)

    print(f"Database: {db_path}")
    ok = run_source_checks(atools_src, lnm_src)
    ok = run_db_checks(db_path, args.sample_airport) and ok

    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
