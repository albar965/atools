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

-- *********************************************************************************************
-- Add VOR, NDB and other dummy waypoints that are needed for procedure display
-- in Little Navmap versions below or equal 2.4.5
-- *********************************************************************************************

drop table if exists waypoint_temp;

create table waypoint_temp
(
  waypoint_id integer primary key,
  ident varchar(5),                   -- ICAO ident
  region varchar(2),                  -- ICAO two letter region identifier
  type varchar(15),                   -- see enum atools::fs::bgl::nav::WaypointType
                                      -- N = NDB, OA = off airway, V = VOR, WN = named waypoint, WU = unnamed waypoint
  lonx double not null,
  laty double not null
);

-- **********************************************************
-- Add VOR waypoints that are referenced by procedures

insert into waypoint_temp (ident, region, type, lonx, laty)
select
  a.waypoint_identifier as ident, a.waypoint_icao_code as region, 'V' as type,
  a.waypoint_longitude as lonx, a.waypoint_latitude as laty
from (
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_iaps
  union
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_sids
  union
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_stars
) a join tbl_vhfnavaids v on
  a.waypoint_identifier = v.vor_identifier and a.waypoint_icao_code = v.icao_code and
  a.waypoint_latitude = v.vor_latitude and a.waypoint_longitude = v.vor_longitude
-- Get all except ILS and MLS
where substr(v.navaid_class, 2, 1) not in ('I', 'N', 'P');

insert into waypoint_temp (ident, region, type, lonx, laty)
select
  a.ident as ident, v.icao_code as region, 'V' as type,
  a.lonx as lonx, a.laty as laty
from (
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_iaps
  union
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_sids
  union
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_stars
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_iaps
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_sids
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_stars
) a join tbl_vhfnavaids v on
  a.ident = v.vor_identifier and a.laty = v.vor_latitude and a.lonx = v.vor_longitude
-- Get all except ILS and MLS
where substr(v.navaid_class, 2, 1) not in ('I', 'N', 'P');


-- **********************************************************
-- Add terminal NDB waypoints that are referenced by procedures

insert into waypoint_temp (ident, region, type, lonx, laty)
select
  a.waypoint_identifier as ident, a.waypoint_icao_code as region, 'N' as type,
  a.waypoint_longitude as lonx, a.waypoint_latitude as laty
from (
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_iaps
  union
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_sids
  union
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_stars
) a join tbl_terminal_ndbnavaids v on
  a.waypoint_identifier = v.ndb_identifier and a.waypoint_icao_code = v.icao_code and
  a.waypoint_latitude = v.ndb_latitude and a.waypoint_longitude = v.ndb_longitude;

insert into waypoint_temp (ident, region, type, lonx, laty)
select
  a.ident as ident, v.icao_code as region, 'N' as type,
  a.lonx as lonx, a.laty as laty
from (
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_iaps
  union
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_sids
  union
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_stars
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_iaps
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_sids
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_stars
) a join tbl_terminal_ndbnavaids v on
  a.ident = v.ndb_identifier and a.laty = v.ndb_latitude and a.lonx = v.ndb_longitude;


-- **********************************************************
-- Add enroute NDB waypoints that are referenced by procedures

insert into waypoint_temp (ident, region, type, lonx, laty)
select
  a.waypoint_identifier as ident, a.waypoint_icao_code as region, 'N' as type,
  a.waypoint_longitude as lonx, a.waypoint_latitude as laty
from (
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_iaps
  union
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_sids
  union
    select waypoint_identifier, waypoint_icao_code, waypoint_latitude, waypoint_longitude from tbl_stars
) a join tbl_enroute_ndbnavaids v on
  a.waypoint_identifier = v.ndb_identifier and a.waypoint_icao_code = v.icao_code and
  a.waypoint_latitude = v.ndb_latitude and a.waypoint_longitude = v.ndb_longitude;

insert into waypoint_temp (ident, region, type, lonx, laty)
select
  a.ident as ident, v.icao_code as region, 'N' as type,
  a.lonx as lonx, a.laty as laty
from (
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_iaps
  union
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_sids
  union
    select recommanded_navaid as ident, recommanded_navaid_latitude as laty, recommanded_navaid_longitude as lonx from tbl_stars
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_iaps
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_sids
  union
    select center_waypoint as ident, center_waypoint_latitude as laty, center_waypoint_longitude as lonx from tbl_stars
) a join tbl_enroute_ndbnavaids v on
  a.ident = v.ndb_identifier and a.laty = v.ndb_latitude and a.lonx = v.ndb_longitude;


-- **********************************************************
-- Now delete all waypoints which already exist in the waypoint table

-- Delete all duplicate waypoints that are at the same position having same name, region and type
-- 1 deg manhattan distance about 60-100 nm at the equator
delete from waypoint_temp where waypoint_id in (
  select distinct w1.waypoint_id
  from waypoint_temp w1
  join waypoint_temp w2 on  w1.ident = w2.ident and w1.region = w2.region and w1.type = w2.type
  where w1.waypoint_id < w2.waypoint_id and (abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.00000001);

delete from waypoint_temp where waypoint_id in (
  select w1.waypoint_id from waypoint_temp w1
  join waypoint w2 on  w1.ident = w2.ident and w1.region = w2.region and w1.type = w2.type
  where (abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.00000001);

-- **********************************************************
-- Copy all waypoints from temp table to waypoint table

insert into waypoint (file_id, nav_id, ident, region, airport_id, artificial, type,
                      num_victor_airway, num_jet_airway, mag_var, lonx, laty)
select 1 as file_id, null as nav_id, ident, region, null as airport_id, 2 as artificial, type,
                      0 as num_victor_airway, 0 as num_jet_airway, 0 as mag_var, lonx, laty
from waypoint_temp;
