-- *****************************************************************************
-- Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

-- --------------------------------------------------------------------------------------------
-- Create navaid airport connections by populating airport_id of respective navaid column
-- --------------------------------------------------------------------------------------------

----------------------------------------------------------
-- airport_id for table waypoint -------------------------
drop table if exists tmp_waypoint_ap;

create table tmp_waypoint_ap as
select distinct a.airport_id, w.waypoint_id
from waypoint w join src.tbl_terminal_waypoints tw on
  w.ident = tw.waypoint_identifier and
  w.region = tw.icao_code and
  w.lonx = tw.waypoint_longitude and w.laty = tw.waypoint_latitude
join airport a on tw.region_code = a.ident ;

create index idx_tmp_waypoint_ap on tmp_waypoint_ap(airport_id);
create index idx_tmp_waypoint_wp on tmp_waypoint_ap(waypoint_id);

update waypoint set airport_id =
(select t.airport_id from tmp_waypoint_ap t
where t.waypoint_id = waypoint.waypoint_id)
where waypoint.airport_id is null;

----------------------------------------------------------
-- airport_id for table ndb ------------------------------
drop table if exists tmp_ndb_ap;

create table tmp_ndb_ap as
select distinct a.airport_id, n.ndb_id
from ndb n join src.tbl_terminal_ndbnavaids tn on
  n.ident = tn.ndb_identifier and
  n.region = tn.icao_code and
  n.lonx = tn.ndb_longitude and
  n.laty = tn.ndb_latitude
join airport a on tn.airport_identifier = a.ident;

create index idx_tmp_ndb_ap on tmp_ndb_ap(airport_id);
create index idx_tmp_ndb_wp on tmp_ndb_ap(ndb_id);

update ndb set airport_id =
(select t.airport_id from tmp_ndb_ap t
where t.ndb_id = ndb.ndb_id)
where ndb.airport_id is null;

----------------------------------------------------------
-- airport_id for table vor ------------------------------
drop table if exists tmp_vor_ap;

create table tmp_vor_ap as
select distinct a.airport_id, v.vor_id
from vor v join src.tbl_vhfnavaids tv on
  v.ident = tv.vor_identifier and
  v.region = tv.icao_code and
  v.lonx = tv.vor_longitude and
  v.laty = tv.vor_latitude
join airport a on tv.airport_identifier = a.ident;

create index idx_tmp_vor_ap on tmp_vor_ap(airport_id);
create index idx_tmp_vor_wp on tmp_vor_ap(vor_id);

update vor set airport_id =
(select t.airport_id from tmp_vor_ap t
where t.vor_id = vor.vor_id)
where vor.airport_id is null;
