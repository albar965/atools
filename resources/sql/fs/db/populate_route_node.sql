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

-- *************************************************************
-- Fill the route_node tables with navaids
-- *************************************************************

----------------------------------------------------------------
-- Populate route_node_radio table with VOR and NDB -----------------------------------

delete from route_node_radio;

insert into route_node_radio (nav_id, type, range, lonx, laty)
select vor_id as nav_id,
  case
    when dme_only = 1 then 3         -- DME
    when dme_altitude is null then 1 -- VOR
    else 2                           -- VORDME
  end as type,
  (range * 1852.216) as range, lonx, laty
from vor;

insert into route_node_radio (nav_id, type, range, lonx, laty)
select ndb_id as nav_id, 4 as type, (range  * 1852.216) as range, lonx, laty
from ndb;

create index if not exists idx_route_node_radio_lonx on route_node_radio(lonx);
create index if not exists idx_route_node_radio_laty on route_node_radio(laty);

-- Populate route_node_airway table with waypoints -----------------------------------

delete from route_node_airway;

-- Type field: airway type is in bits 4-7 and a subtype (vor, etc.) is stored in bits 0-3
insert into route_node_airway (nav_id, type, lonx, laty)
select w.waypoint_id as nav_id,
  case when w.num_victor_airway > 0 and w.num_jet_airway = 0 then
  -- Waypoint victor
  case when w.type = 'NDB' then 5 * 16 + 4         -- victor + NDB
    when w.type = 'VOR' then
    case when v.dme_only = 1 then 5 * 16 + 3     -- victor + DME
    when v.dme_altitude is null then 5 * 16 + 1  -- victor + VOR
    else 5 * 16 + 2                              -- victor + VORDME
    end
  else 5 * 16                                      -- victor + Waypoint only
  end
  when w.num_victor_airway = 0 and w.num_jet_airway > 0 then
  -- Waypoint jet
  case when w.type = 'NDB' then 6 * 16 + 4         -- jet + NDB
  when w.type = 'VOR' then
    case when v.dme_only = 1 then 6 * 16 + 3     -- jet + DME
    when v.dme_altitude is null then 6 * 16 + 1  -- jet + VOR
    else 6 * 16 + 2                              -- jet + VORDME
    end
    else 6 * 16                                      -- jet + Waypoint only
    end
  when w.num_victor_airway > 0 and w.num_jet_airway > 0 then
  -- Waypoint both
  case when w.type = 'NDB' then 7 * 16 + 4         -- both + NDB
  when w.type = 'VOR' then
    case when v.dme_only = 1 then 7 * 16 + 3     -- both + DME
    when v.dme_altitude is null then 7 * 16 + 1  -- both + VOR
    else 7 * 16 + 2                              -- both + VORDME
    end
    else 7 * 16                                      -- both + Waypoint only
    end
  else null -- Should never happen - let it fail with not null
  end as type, w.lonx, w.laty
from waypoint w left outer join vor v on w.nav_id = v.vor_id
where w.num_victor_airway > 0 or w.num_jet_airway > 0;
