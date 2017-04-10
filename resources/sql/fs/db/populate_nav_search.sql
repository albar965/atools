-- *****************************************************************************
-- Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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

-- *************************************************************
-- Fill the nav_search table which is used to search for
-- all navigation aids mixed
-- *************************************************************

delete from nav_search;

-- Insert waypoints into nav_search table
insert into nav_search (waypoint_id, waypoint_nav_id, file_id, ident, region, airport_id, airport_ident,
  type, nav_type, waypoint_num_victor_airway, waypoint_num_jet_airway, scenery_local_path, bgl_filename, mag_var, lonx, laty)
select w.waypoint_id, w.nav_id, w.file_id, w.ident, w.region, w.airport_id, a.ident as airport_ident,
  w.type, 'W', w.num_victor_airway, w.num_jet_airway, s.local_path, f.filename, w.mag_var, w.lonx, w.laty
from waypoint w
join bgl_file f on f.bgl_file_id = w.file_id
join scenery_area s on f.scenery_area_id = s.scenery_area_id
left outer join airport a on w.airport_id = a.airport_id;

-- Insert NDBs into nav_search table
insert into nav_search (ndb_id, file_id, airport_id, airport_ident, ident, name, region, range, type, nav_type, frequency,
  scenery_local_path, bgl_filename, mag_var, altitude, lonx, laty)
select n.ndb_id, n.file_id, n.airport_id, a.ident as airport_ident, n.ident, n.name, n.region, n.range, 'N' || n.type, 'N', n.frequency,
  s.local_path, f.filename, n.mag_var, n.altitude, n.lonx, n.laty
from ndb n
join bgl_file f on f.bgl_file_id = n.file_id
join scenery_area s on f.scenery_area_id = s.scenery_area_id
left outer join airport a on n.airport_id = a.airport_id;

-- Insert VORs into nav_search table
insert into nav_search (vor_id, file_id, airport_id, airport_ident, ident, name, region, range, type, nav_type, frequency, channel,
  scenery_local_path, bgl_filename, mag_var, altitude, lonx, laty)
select v.vor_id, v.file_id, v.airport_id, a.ident as airport_ident, v.ident, v.name, v.region, v.range,
  case
    when v.type = 'VTH' or v.type = 'H' then 'VH'              -- VORTAC
    when v.type = 'VTL' or v.type = 'L' then 'VL'              -- VORTAC
    when v.type = 'VTT' or v.type = 'T' then 'VT'              -- VORTAC
  else
    v.type
  end as type,
  case
    when v.type = 'TC' and dme_only = 0 then 'TC'              -- TACAN
    when v.type = 'TC' and dme_only = 1 then 'TCD'             -- TACAN DME only
    when v.type like 'VT%' and dme_only = 0 then 'VT'          -- VORTAC
    when v.type like 'VT%' and dme_only = 1 then 'VTD'         -- VORTAC DME only
    when dme_only = 1 then 'D'                                 -- DME
    when dme_only = 0 and dme_altitude is not null then 'VD'   -- VORDME
    when dme_only = 0 and dme_altitude is null then 'V'        -- VOR
  else
    'U'
  end as nav_type,
  v.frequency * 10 as frequency,
  v.channel,
  s.local_path, f.filename, v.mag_var, v.altitude, v.lonx, v.laty
from vor v
join bgl_file f on f.bgl_file_id = v.file_id
join scenery_area s on f.scenery_area_id = s.scenery_area_id
left outer join airport a on v.airport_id = a.airport_id;

-- Finish with creating indexe
create index if not exists idx_nav_search_airport_ident on nav_search(airport_ident);
create index if not exists idx_nav_search_ident on nav_search(ident);
create index if not exists idx_nav_search_name on nav_search(name);
create index if not exists idx_nav_search_region on nav_search(region);
create index if not exists idx_nav_search_range on nav_search(range);
create index if not exists idx_nav_search_type on nav_search(type);
create index if not exists idx_nav_search_nav_type on nav_search(nav_type);
create index if not exists idx_nav_search_scenery_local_path on nav_search(scenery_local_path);
create index if not exists idx_nav_search_bgl_filename on nav_search(bgl_filename);
create index if not exists idx_nav_search_waypoint_num_vairway on nav_search(waypoint_num_victor_airway);
create index if not exists idx_nav_search_waypoint_num_jairway on nav_search(waypoint_num_jet_airway);
