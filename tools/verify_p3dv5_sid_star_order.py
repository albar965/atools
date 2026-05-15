#!/usr/bin/env python3
"""Verify P3D v5 GPS SID/STAR import and transition leg order.

This helper expects a Little Navmap SQLite database compiled from P3D v5
stock scenery after the atools import fix. It checks the transition-only
GPS overlay SID/STAR shape and the database ordering consumed by LNM.
"""

from __future__ import annotations

import argparse
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


def query_one(con: sqlite3.Connection, sql: str, params: tuple = ()) -> sqlite3.Row:
    row = con.execute(sql, params).fetchone()
    if row is None:
        raise RuntimeError(f"query returned no rows: {sql}")
    return row


def print_rows(title: str, rows: list[sqlite3.Row]) -> None:
    print(title)
    for row in rows:
        print("  " + ", ".join(f"{key}={row[key]}" for key in row.keys()))


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

    ok = True
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

    sid_anchor = query_one(
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
               sum(case when first.fix_type = 'R'
                        or (first.fix_type is null and first.type in ({",".join("?" for _ in NO_FIX_DEPARTURE_TYPES)}))
                        then 1 else 0 end) as first_runway_or_implicit,
               sum(case when last.fix_type = 'R'
                        or (last.fix_type is null and last.type in ({",".join("?" for _ in NO_FIX_DEPARTURE_TYPES)}))
                        then 1 else 0 end) as last_runway_or_implicit
        from first_last fl
        join transition_leg first on first.transition_leg_id = fl.first_id
        join transition_leg last on last.transition_leg_id = fl.last_id
        """,
        tuple(sorted(NO_FIX_DEPARTURE_TYPES)) * 2,
    )
    print(
        "SID order diagnostics: "
        f"transitions={sid_anchor['transitions']}, "
        f"first runway/implicit={sid_anchor['first_runway_or_implicit']}, "
        f"last runway/implicit={sid_anchor['last_runway_or_implicit']}"
    )

    star_if = query_one(
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
               sum(case when first.type in ('IF', 'FD', 'FC') then 1 else 0 end) as first_entry_like,
               sum(case when last.type in ('IF', 'FD', 'FC') then 1 else 0 end) as last_entry_like
        from first_last fl
        join transition_leg first on first.transition_leg_id = fl.first_id
        join transition_leg last on last.transition_leg_id = fl.last_id
        """,
    )
    print(
        "STAR order diagnostics: "
        f"transitions={star_if['transitions']}, "
        f"first entry-like={star_if['first_entry_like']}, "
        f"last entry-like={star_if['last_entry_like']}"
    )

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

    con.close()
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--db", required=True, type=Path, help="Compiled Little Navmap SQLite database")
    parser.add_argument("--atools-src", type=Path, help="Optional atools source tree for source order checks")
    parser.add_argument("--lnm-src", type=Path, help="Optional Little Navmap source tree for read order checks")
    parser.add_argument("--sample-airport", action="append", default=["EDDF", "ZBAA"])
    args = parser.parse_args()

    if not args.db.exists():
        print(f"FAIL database does not exist: {args.db}", file=sys.stderr)
        return 2

    ok = run_source_checks(args.atools_src, args.lnm_src)
    ok = run_db_checks(args.db, args.sample_airport) and ok

    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
