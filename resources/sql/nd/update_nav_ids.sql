-- *****************************************************************************
-- Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.
-- ****************************************************************************/

----------------------------------------------------------------
-- Update navigation references for waypoint -------------------

update waypoint set nav_id =
(
  select v.vor_id
  from vor v
  where waypoint.type = 'VOR' and
  waypoint.ident = v.ident and
  waypoint.region = v.region
)
where waypoint.type = 'VOR';

update waypoint set nav_id =
(
  select n.ndb_id
  from ndb n
  where waypoint.type = 'NDB' and
  waypoint.ident = n.ident and
  waypoint.region = n.region
)
where waypoint.type = 'NDB';

----------------------------------------------------------------
-- Update navigation references for approach -------------------

update approach set fix_nav_id =
(
  select v.vor_id
  from vor v
  where approach.fix_type = 'VOR' and approach.fix_ident = v.ident and approach.fix_region = v.region
) where approach.fix_type = 'VOR';

update approach set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where approach.fix_type = 'NDB' and approach.fix_ident = n.ident and approach.fix_region = n.region
) where approach.fix_type = 'NDB';

update approach set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where approach.fix_type = 'WAYPOINT' and approach.fix_ident = w.ident and approach.fix_region = w.region
) where approach.fix_type = 'WAYPOINT';

-- Terminals -----------------------------------
update approach set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where approach.fix_type = 'TERMINAL_NDB' and approach.fix_ident = n.ident and
  approach.fix_region = n.region and a.ident = approach.fix_airport_ident
) where approach.fix_type = 'TERMINAL_NDB';

update approach set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where approach.fix_type = 'TERMINAL_WAYPOINT' and approach.fix_ident = w.ident and
  approach.fix_region = w.region and a.ident = approach.fix_airport_ident
) where approach.fix_type = 'TERMINAL_WAYPOINT';

----------------------------------------------------------------
-- Update navigation references for transition -------------------

update transition set fix_nav_id =
(
  select v.vor_id
  from vor v
  where transition.fix_type = 'VOR' and transition.fix_ident = v.ident and transition.fix_region = v.region
) where transition.fix_type = 'VOR';

update transition set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where transition.fix_type = 'NDB' and transition.fix_ident = n.ident and transition.fix_region = n.region
) where transition.fix_type = 'NDB';

update transition set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where transition.fix_type = 'WAYPOINT' and transition.fix_ident = w.ident and transition.fix_region = w.region
) where transition.fix_type = 'WAYPOINT';

-- Terminals -----------------------------------

update transition set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where transition.fix_type = 'TERMINAL_NDB' and transition.fix_ident = n.ident and
  transition.fix_region = n.region and a.ident = transition.fix_airport_ident
) where transition.fix_type = 'TERMINAL_NDB';

update transition set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where transition.fix_type = 'TERMINAL_WAYPOINT' and transition.fix_ident = w.ident and
  transition.fix_region = w.region and a.ident = transition.fix_airport_ident
) where transition.fix_type = 'TERMINAL_WAYPOINT';

----------------------------------------------------------------
-- Update navigation references for approach legs --------------

update approach_leg set fix_nav_id =
(
  select v.vor_id
  from vor v
  where approach_leg.fix_type = 'VOR' and approach_leg.fix_ident = v.ident and approach_leg.fix_region = v.region
) where approach_leg.fix_type = 'VOR';

update approach_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where approach_leg.fix_type = 'NDB' and approach_leg.fix_ident = n.ident and approach_leg.fix_region = n.region
) where approach_leg.fix_type = 'NDB';

update approach_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where approach_leg.fix_type = 'WAYPOINT' and approach_leg.fix_ident = w.ident and approach_leg.fix_region = w.region
) where approach_leg.fix_type = 'WAYPOINT';

-- Terminals -----------------------------------
update approach_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where approach_leg.fix_type = 'TERMINAL_NDB' and approach_leg.fix_ident = n.ident and
  approach_leg.fix_region = n.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.fix_type = 'TERMINAL_NDB';

update approach_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where approach_leg.fix_type = 'TERMINAL_WAYPOINT' and approach_leg.fix_ident = w.ident and
  approach_leg.fix_region = w.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.fix_type = 'TERMINAL_WAYPOINT';

----------------------------------------------------------------
-- Update navigation references for transition legs ------------

update transition_leg set fix_nav_id =
(
  select v.vor_id
  from vor v
  where transition_leg.fix_type = 'VOR' and transition_leg.fix_ident = v.ident and transition_leg.fix_region = v.region
) where transition_leg.fix_type = 'VOR';

update transition_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where transition_leg.fix_type = 'NDB' and transition_leg.fix_ident = n.ident and transition_leg.fix_region = n.region
) where transition_leg.fix_type = 'NDB';

update transition_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where transition_leg.fix_type = 'WAYPOINT' and transition_leg.fix_ident = w.ident and transition_leg.fix_region = w.region
) where transition_leg.fix_type = 'WAYPOINT';

-- Terminals -----------------------------------

update transition_leg set fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where transition_leg.fix_type = 'TERMINAL_NDB' and transition_leg.fix_ident = n.ident and
  transition_leg.fix_region = n.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.fix_type = 'TERMINAL_NDB';

update transition_leg set fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where transition_leg.fix_type = 'TERMINAL_WAYPOINT' and transition_leg.fix_ident = w.ident and
  transition_leg.fix_region = w.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.fix_type = 'TERMINAL_WAYPOINT';

---------------------------------------------------------------
-- Update navigation references for approach legs --------------

update approach_leg set recommended_fix_nav_id =
(
  select v.vor_id
  from vor v
  where approach_leg.recommended_fix_type = 'VOR' and approach_leg.recommended_fix_ident = v.ident and
  approach_leg.recommended_fix_region = v.region
) where approach_leg.recommended_fix_type = 'VOR';

update approach_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where approach_leg.recommended_fix_type = 'NDB' and approach_leg.recommended_fix_ident = n.ident and
  approach_leg.recommended_fix_region = n.region
) where approach_leg.recommended_fix_type = 'NDB';

update approach_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where approach_leg.recommended_fix_type = 'WAYPOINT' and approach_leg.recommended_fix_ident = w.ident and
  approach_leg.recommended_fix_region = w.region
) where approach_leg.recommended_fix_type = 'WAYPOINT';

-- Terminals -----------------------------------
update approach_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where approach_leg.recommended_fix_type = 'TERMINAL_NDB' and approach_leg.recommended_fix_ident = n.ident and
  approach_leg.recommended_fix_region = n.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.recommended_fix_type = 'TERMINAL_NDB';

update approach_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where approach_leg.recommended_fix_type = 'TERMINAL_WAYPOINT' and approach_leg.recommended_fix_ident = w.ident and
  approach_leg.recommended_fix_region = w.region and a.ident = approach_leg.fix_airport_ident
) where approach_leg.recommended_fix_type = 'TERMINAL_WAYPOINT';

----------------------------------------------------------------
-- Update navigation references for transition legs ------------

update transition_leg set recommended_fix_nav_id =
(
  select v.vor_id
  from vor v
  where transition_leg.recommended_fix_type = 'VOR' and transition_leg.recommended_fix_ident = v.ident and
  transition_leg.recommended_fix_region = v.region
) where transition_leg.recommended_fix_type = 'VOR';

update transition_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n
  where transition_leg.recommended_fix_type = 'NDB' and transition_leg.recommended_fix_ident = n.ident and
  transition_leg.recommended_fix_region = n.region
) where transition_leg.recommended_fix_type = 'NDB';

update transition_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w
  where transition_leg.recommended_fix_type = 'WAYPOINT' and transition_leg.recommended_fix_ident = w.ident and
  transition_leg.recommended_fix_region = w.region
) where transition_leg.recommended_fix_type = 'WAYPOINT';

-- Terminals -----------------------------------

update transition_leg set recommended_fix_nav_id =
(
  select n.ndb_id
  from ndb n join airport a on n.airport_id = a.airport_id
  where transition_leg.recommended_fix_type = 'TERMINAL_NDB' and transition_leg.recommended_fix_ident = n.ident and
  transition_leg.recommended_fix_region = n.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.recommended_fix_type = 'TERMINAL_NDB';

update transition_leg set recommended_fix_nav_id =
(
  select w.waypoint_id
  from waypoint w join airport a on w.airport_id = a.airport_id
  where transition_leg.recommended_fix_type = 'TERMINAL_WAYPOINT' and transition_leg.recommended_fix_ident = w.ident and
  transition_leg.recommended_fix_region = w.region and a.ident = transition_leg.fix_airport_ident
) where transition_leg.recommended_fix_type = 'TERMINAL_WAYPOINT';


----------------------------------------------------------------
-- Update number of ILS runway ends in aiport ------------------

drop table if exists temp_ap_num_ils;

create table temp_ap_num_ils as
select cast(ap_id as integer) as ap_id, cast(count(1) as integer) as cnt from (
select r.airport_id as ap_id, primary_end_id as end_id from runway r
join ils i on r.primary_end_id = i.loc_runway_end_id
where i.gs_range is not null
union
select r2.airport_id as ap_id,secondary_end_id as end_id from runway r2
join ils i2 on r2.secondary_end_id = i2.loc_runway_end_id
where i2.gs_range is not null
) group by ap_id;

create index if not exists idx_temp_ap_num_ils on temp_ap_num_ils(ap_id);

update airport set num_runway_end_ils = (
select r.cnt
from temp_ap_num_ils r where r.ap_id = airport.airport_id);

drop table if exists temp_ap_num_ils;

update airport set num_runway_end_ils = 0 where num_runway_end_ils is null;


----------------------------------------------------------------
-- Populate nav_search table -----------------------------------

delete from nav_search;

-- Insert waypoints into nav_search table
insert into nav_search (waypoint_id, waypoint_nav_id, file_id, ident, region, airport_id, airport_ident,
type, nav_type, scenery_local_path,bgl_filename, mag_var, lonx, laty)
select w.waypoint_id, w.nav_id, w.file_id, w.ident, w.region, w.airport_id, a.ident as airport_ident,
w.type, 'WAYPOINT', s.local_path, f.filename, w.mag_var, w.lonx, w.laty
from waypoint w
join bgl_file f on f.bgl_file_id = w.file_id
join scenery_area s on f.scenery_area_id = s.scenery_area_id
left outer join airport a on w.airport_id = a.airport_id;

-- Insert NDBs into nav_search table
insert into nav_search (ndb_id, file_id, airport_id, airport_ident, ident, name, region, range, type, nav_type, frequency,
scenery_local_path, bgl_filename, mag_var, altitude, lonx, laty)
select n.ndb_id, n.file_id, n.airport_id, a.ident as airport_ident, n.ident, n.name, n.region, n.range, n.type, 'NDB', n.frequency,
s.local_path, f.filename, n.mag_var, n.altitude, n.lonx, n.laty
from ndb n
join bgl_file f on f.bgl_file_id = n.file_id
join scenery_area s on f.scenery_area_id = s.scenery_area_id
left outer join airport a on n.airport_id = a.airport_id;

-- Insert VORs into nav_search table
insert into nav_search (vor_id, file_id, airport_id, airport_ident, ident, name, region, range, type, nav_type, frequency,
scenery_local_path, bgl_filename, mag_var, altitude, lonx, laty)
select v.vor_id, v.file_id, v.airport_id, a.ident as airport_ident, v.ident, v.name, v.region, v.range, v.type,
case
  when dme_only = 1 then 'DME'
  when dme_only = 0 and dme_altitude is not null then 'VORDME'
  when dme_only = 0 and dme_altitude is null then 'VOR'
else
  'UNKNOWN'
end as nav_type,
v.frequency * 10 as frequency,
s.local_path, f.filename, v.mag_var, v.altitude, v.lonx, v.laty
from vor v
join bgl_file f on f.bgl_file_id = v.file_id
join scenery_area s on f.scenery_area_id = s.scenery_area_id
left outer join airport a on v.airport_id = a.airport_id;

create index if not exists idx_nav_search_airport_ident on nav_search(airport_ident);
create index if not exists idx_nav_search_ident on nav_search(ident);
create index if not exists idx_nav_search_name on nav_search(name);
create index if not exists idx_nav_search_region on nav_search(region);
create index if not exists idx_nav_search_range on nav_search(range);
create index if not exists idx_nav_search_type on nav_search(type);
create index if not exists idx_nav_search_nav_type on nav_search(nav_type);
create index if not exists idx_nav_search_scenery_local_path on nav_search(scenery_local_path);
create index if not exists idx_nav_search_bgl_filename on nav_search(bgl_filename);









