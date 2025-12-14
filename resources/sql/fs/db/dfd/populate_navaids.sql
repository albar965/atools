-- *****************************************************************************
-- Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

-- This script fills the navaid tables from an attached DFD dataset with the name "src"
-- file_id uses hardcoded value 1 for all datasets

-- Duplicates are inserted and will be removed later

-- *********************************************************************************************
-- Fill VOR table
-- *********************************************************************************************

delete from vor;

-- X-Plane definition
-- 25, 40 and 130 correspond to VORs classified as terminal, low and high.
-- 125 range is where the VOR has no published term/low/high classification.
-- These VORs might have the power output of a high VOR, but are not tested/certified to fulfill the
-- high altitude SVV.

insert into vor (file_id, ident, name, region, type, frequency, channel, range, mag_var,
                 dme_only, dme_altitude, dme_lonx, dme_laty, altitude, lonx, laty)
select
    1 as file_id,
    vor_identifier as ident,
    vor_name as name,
    icao_code as region,
    -- Type -----------------
    -- VOR, VORDME and DME
    case when substr(navaid_class, 1, 2) in ('V ', 'VD', ' D') then
        case substr(navaid_class, 3, 1)
        when 'U' then
        -- Range class is unknown - create from given range use H if not available
        case when range < 30 then 'T' when range < 50 then 'L' else 'H' end
        else substr(navaid_class, 3, 1)
        end
    -- VORTAC normal and military
    when substr(navaid_class, 1, 2) in ('VT', 'VM') then
        case substr(navaid_class, 3, 1)
        when 'U' then
        -- Range class is unknown - create from given range use H if not available
        case when range < 30 then 'VTT' when range < 50 then 'VTL' else 'VTH' end
        else 'VT' || substr(navaid_class, 3, 1)
        end
    -- TACAN normal and military
    when substr(navaid_class, 1, 2) in (' T', ' M') then 'TC' else 'INVALID "' || navaid_class || '"'
    end as type,
    vor_frequency * 1000 as frequency,
    null as channel, -- Calculated later in C++ code
    range,
    station_declination as mag_var,
    -- Set flag for DME only VOR
    case when substr(navaid_class, 1, 2) =' D' then 1 else 0 end as dme_only,
    -- Exclude DME data if VOR only
    case when substr(navaid_class, 2, 1) = (' ') then null else dme_elevation end as dme_altitude,
    case when substr(navaid_class, 2, 1) = (' ') then null else dme_longitude end as dme_lonx,
    case when substr(navaid_class, 2, 1) = (' ') then null else dme_latitude end as dme_laty,
    -- Use DME elevation as elevation - VOR without DME have no elevation
    case when substr(navaid_class, 1, 2) ='V ' and dme_elevation = 0  then null else dme_elevation end as altitude,
    vor_longitude as lonx,
    vor_latitude as laty
from tbl_vhfnavaids
-- Get all except ILS and MLS
where substr(navaid_class, 2, 1) not in ('I', 'N', 'P');


-- *********************************************************************************************
-- Fill NDB table and merge enroute and terminal NDBs
-- *********************************************************************************************

delete from ndb;

insert into ndb (file_id, ident, name, region, type, frequency, range, mag_var, altitude, lonx, laty)
select
  1 as file_id,
  ndb_identifier as ident,
  ndb_name as name,
  icao_code as region,
   --  CP MH H HH
  case
    when substr(navaid_class, 3, 1) = 'L' then 'CP'
    when substr(navaid_class, 3, 1) = 'M' then 'MH'
    when substr(navaid_class, 3, 1) = 'H' then 'H'
    else null
  end as type,
  ndb_frequency * 100 as frequency,
  case
    when substr(navaid_class, 3, 1) = 'L' then 15
    when substr(navaid_class, 3, 1) = 'M' then 25
    when substr(navaid_class, 3, 1) = 'H' then 50
    else null
  end as range,
  0 as  mag_var, -- Calculated later in C++ code
  null as altitude, -- Not available
  ndb_longitude as lonx,
  ndb_latitude as laty
from (
select  area_code, icao_code, ndb_identifier, ndb_name, ndb_frequency, navaid_class, ndb_latitude, ndb_longitude
from tbl_enroute_ndbnavaids
union
select  area_code, icao_code, ndb_identifier, ndb_name, ndb_frequency, navaid_class, ndb_latitude, ndb_longitude
from tbl_terminal_ndbnavaids);

-- *********************************************************************************************
-- Fill ILS table
-- *********************************************************************************************

delete from ils;

-- Plain ILS, SDF, LDA and localizers **********************************

insert into ils ( ident, region, type, frequency, range, mag_var, has_backcourse,
  dme_range, dme_altitude, dme_lonx, dme_laty,
  gs_range, gs_pitch, gs_altitude, gs_lonx, gs_laty,
  loc_airport_ident, loc_runway_name, loc_heading, loc_width,
  end1_lonx, end1_laty, end_mid_lonx, end_mid_laty, end2_lonx, end2_laty, altitude, lonx, laty)
select
  l.llz_identifier as ident,
  l.icao_code as region,
  ils_mls_gls_category as type,
  l.llz_frequency * 1000 as frequency,
  27 as range, -- default range used by FSX
  l.station_declination as mag_var,
  0 as has_backcourse, -- Not available
  d.range as dme_range,
  d.dme_elevation as dme_altitude,
  d.dme_longitude as dme_lonx,
  d.dme_latitude as dme_laty,
  27 as gs_range,
  l.gs_angle as gs_pitch,
  l.gs_elevation as gs_altitude,
  l.gs_longitude as gs_lonx,
  l.gs_latitude as gs_laty,
  l.airport_identifier as loc_airport_ident,
  substr(l.runway_identifier, 3) as loc_runway_name, -- Strip off "RW" prefix
  l.llz_bearing + l.station_declination as loc_heading, -- Magnetic to true
  l.llz_width as loc_width,
  0 as end1_lonx,  -- All geometry is calculated later
  0 as end1_laty,
  0 as end_mid_lonx,
  0 as end_mid_laty,
  0 as end2_lonx,
  0 as end2_laty,
  0 as altitude,
  l.llz_longitude as lonx,
  l.llz_latitude as laty
from tbl_localizers_glideslopes l left outer join tbl_vhfnavaids d on
  l.llz_identifier = d.vor_identifier and l.icao_code = d.icao_code;

-- GLS approach stations **********************************

insert into ils ( ident, region, type, frequency, range, mag_var, has_backcourse,
  gs_range, gs_pitch, gs_altitude, gs_lonx, gs_laty,
  loc_airport_ident, loc_runway_name, loc_heading,
  end1_lonx, end1_laty, end_mid_lonx, end_mid_laty, end2_lonx, end2_laty, altitude, lonx, laty)
select
  g.gls_ref_path_identifier as ident,
  g.icao_code as region,
  'G' as type,
  g.gls_channel as frequency,
  27 as range, -- default range used by FSX
  g.magentic_variation as mag_var,
  0 as has_backcourse, -- Not available
  27 as gs_range,
  g.gls_approach_slope as gs_pitch,
  g.station_elevation as gs_altitude,
  g.station_longitude as gs_lonx,
  g.station_latitude as gs_laty,
  g.airport_identifier as loc_airport_ident,
  substr(g.runway_identifier, 3) as loc_runway_name, -- Strip off "RW" prefix
  g.gls_approach_bearing + g.magentic_variation as loc_heading, -- Magnetic to true
  0 as end1_lonx,  -- All geometry is calculated later
  0 as end1_laty,
  0 as end_mid_lonx,
  0 as end_mid_laty,
  0 as end2_lonx,
  0 as end2_laty,
  0 as altitude,
  r.runway_longitude as lonx,
  r.runway_latitude as laty
from tbl_gls g left outer join tbl_runways r on
  g.runway_identifier = r.runway_identifier and g.icao_code = r.icao_code  and g.airport_identifier = r.airport_identifier;

-- *********************************************************************************************
-- Fill Marker table
-- *********************************************************************************************

delete from marker;

insert into marker (file_id, ident, region, type, heading, altitude, lonx, laty)
select
  1 as file_id,
  m.llz_identifier as ident,
  m.icao_code as region,
  case
    when substr(m.marker_type, 2, 2) = 'IM' then 'INNER'
    when substr(m.marker_type, 2, 2) = 'MM' then 'MIDDLE'
    when substr(m.marker_type, 2, 2) = 'OM' then 'OUTER'
    when substr(m.marker_type, 2, 2) = 'BM' then 'BACKCOURSE'
    else null
  end as type,
  coalesce(r.runway_true_bearing, 0) as heading,
  coalesce(r.landing_threshold_elevation, 0) as altitude,
  m.marker_longitude as lonx,
  m.marker_latitude as laty
from tbl_localizer_marker m
-- Get heading and altitude from runway if possible
left outer join tbl_runways r on
  m.airport_identifier = r.airport_identifier and
  m.icao_code = r.icao_code and
  m.runway_identifier = r.runway_identifier;

-- *********************************************************************************************
-- Fill waypoint table
-- *********************************************************************************************

delete from waypoint;

insert into waypoint (file_id, ident, name, region, type, arinc_type, num_victor_airway, num_jet_airway, mag_var, lonx, laty)
select
  1 as file_id,
  waypoint_identifier as ident,
  waypoint_name as name,
  icao_code as region,
  -- N = NDB, OA = off airway, V = VOR, WN = named waypoint, WU = unnamed waypoint
  case when substr(waypoint_type, 1, 1) = 'I' then 'WU' else 'WN' end as type,
  waypoint_type as arinc_type,
  0 as num_victor_airway, -- Calculated later
  0 as num_jet_airway, -- Calculated later
  0 as mag_var, -- Calculated later
  waypoint_longitude as lonx,
  waypoint_latitude as laty
from (
  select area_code, icao_code, waypoint_identifier, waypoint_name, waypoint_type, waypoint_usage,
    waypoint_latitude, waypoint_longitude from tbl_enroute_waypoints
  union
  select area_code, icao_code, waypoint_identifier, waypoint_name, waypoint_type, null as waypoint_usage,
    waypoint_latitude, waypoint_longitude from tbl_terminal_waypoints);

-- *********************************************************************************************
-- Now add VOR, NDB and other dummy waypoints that are needed for routing and procedure display
-- *********************************************************************************************

-- **********************************************************
-- Add VOR waypoints that are referenced by airways

insert into waypoint (file_id, ident, region, artificial, type, num_victor_airway, num_jet_airway, mag_var, lonx, laty)
select
  1 as file_id, a.waypoint_identifier as ident, a.icao_code as region, 1 as artificial, 'V' as type,
  0 as num_victor_airway, 0 as num_jet_airway, 0 as mag_var,
  a.waypoint_longitude as lonx, a.waypoint_latitude as laty
from (select waypoint_identifier, icao_code, waypoint_latitude, waypoint_longitude
      from tbl_enroute_airways) a
join tbl_vhfnavaids v on
  a.waypoint_identifier = v.vor_identifier and a.icao_code = v.icao_code and
  a.waypoint_latitude = v.vor_latitude and a.waypoint_longitude = v.vor_longitude
-- Get all except ILS and MLS
where substr(v.navaid_class, 2, 1) not in ('I', 'N', 'P');

-- **********************************************************
-- Add terminal NDB waypoints that are referenced by airways and procedures

-- A part of the waypoints are not needed by procedures and will be removed later

insert into waypoint (file_id, ident, region, artificial, type, num_victor_airway, num_jet_airway, mag_var, lonx, laty)
select
  1 as file_id, a.waypoint_identifier as ident, a.icao_code as region, 1 as artificial, 'N' as type,
  0 as num_victor_airway, 0 as num_jet_airway, 0 as mag_var,
  a.waypoint_longitude as lonx, a.waypoint_latitude as laty
from (select waypoint_identifier, icao_code, waypoint_latitude, waypoint_longitude
      from tbl_enroute_airways) a
join tbl_terminal_ndbnavaids v on
  a.waypoint_identifier = v.ndb_identifier and a.icao_code = v.icao_code and
  a.waypoint_latitude = v.ndb_latitude and a.waypoint_longitude = v.ndb_longitude;

-- **********************************************************
-- Add enroute NDB waypoints that are referenced by airways and procedures

insert into waypoint (file_id, ident, region, artificial, type, num_victor_airway, num_jet_airway, mag_var, lonx, laty)
select
  1 as file_id, a.waypoint_identifier as ident, a.icao_code as region, 1 as artificial, 'N' as type,
  0 as num_victor_airway, 0 as num_jet_airway, 0 as mag_var,
  a.waypoint_longitude as lonx, a.waypoint_latitude as laty
from (select waypoint_identifier, icao_code, waypoint_latitude, waypoint_longitude from tbl_enroute_airways) a
join tbl_enroute_ndbnavaids v on
  a.waypoint_identifier = v.ndb_identifier and a.icao_code = v.icao_code and
  a.waypoint_latitude = v.ndb_latitude and a.waypoint_longitude = v.ndb_longitude;

-- Delete all duplicate waypoints where type NDB overlaps with type VOR (mostly DME)
-- Leave the NDB waypoints
delete from waypoint where waypoint_id in (
select w2.waypoint_id
from waypoint w1 join waypoint w2 on w1.ident = w2.ident and w1.region = w2.region
where (abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.000001 and
w1.type = 'N' and w2.type='V');

-- =======================================================================
-- Fill enroute holdings

drop table if exists tmp_holding;

create table tmp_holding
(
  airport_ident varchar(5),
  region varchar(2),
  nav_ident varchar(5),
  name varchar(50),
  course double not null,
  turn_direction varchar(1) not null,
  leg_length double,
  leg_time double,
  minimum_altitude double,
  maximum_altitude double,
  speed integer,
  lonx double not null,
  laty double not null
);

-- Remove duplicates having all the same values ================================================
insert into tmp_holding (airport_ident, region, nav_ident, name, course,
  turn_direction, leg_length, leg_time, minimum_altitude, maximum_altitude, speed, lonx, laty)
select distinct
  case when region_code = 'ENRT'  then null else region_code end as airport_ident,
  icao_code as region, waypoint_identifier as nav_ident, holding_name as name,
  inbound_holding_course as course, turn_direction,
  leg_length, leg_time, minimum_altitude, maximum_altitude, holding_speed as speed,
  waypoint_longitude as lonx, waypoint_latitude as laty
from tbl_holdings;

-- Insert holdings at waypoints =======================
insert into holding (file_id, airport_ident, name, nav_id, nav_ident, region, nav_type, course, turn_direction, leg_length, leg_time,
  minimum_altitude, maximum_altitude, speed_limit, lonx, laty)
select 1 as file_id, h.airport_ident, h.name, w.waypoint_id as nav_id, w.ident as nav_ident, h.region, 'W' as nav_type, h.course,
  h.turn_direction, h.leg_length, h.leg_time, h.minimum_altitude, h.maximum_altitude, h.speed, w.lonx, w.laty
from tmp_holding h
join waypoint w on h.nav_ident = w.ident and h.region = w.region and abs(w.lonx - h.lonx) < 0.00001 and abs(w.laty - h.laty) < 0.00001
where w.type like 'W%';

-- Insert holdings at VOR =======================
insert into holding (file_id, airport_ident, name, nav_id, nav_ident, region, nav_type,
  vor_type, vor_dme_only, vor_has_dme, mag_var, course, turn_direction, leg_length, leg_time,
  minimum_altitude, maximum_altitude, speed_limit, mag_var, lonx, laty)
select 1 as file_id, h.airport_ident, h.name, v.vor_id as nav_id, v.ident as nav_ident, h.region,
  'V' as nav_type, v.type as vor_type, v.dme_only as vor_dme_only, case when v.dme_altitude is null then 0 else 1 end as vor_has_dme,
  v.mag_var, h.course,
  h.turn_direction, h.leg_length, h.leg_time, h.minimum_altitude, h.maximum_altitude, h.speed, v.mag_var, v.lonx, v.laty
from tmp_holding h
join vor v on h.nav_ident = v.ident and abs(v.lonx - h.lonx) < 0.00001 and abs(v.laty - h.laty) < 0.00001;

-- Insert holdings at NDB =======================
insert into holding (file_id, airport_ident, name, nav_id, nav_ident, region, nav_type, course, turn_direction, leg_length, leg_time,
  minimum_altitude, maximum_altitude, speed_limit, lonx, laty)
select 1 as file_id, h.airport_ident, h.name, n.ndb_id as nav_id, n.ident as nav_ident, h.region, 'N' as nav_type, h.course,
  h.turn_direction, h.leg_length, h.leg_time, h.minimum_altitude, h.maximum_altitude, h.speed, n.lonx, n.laty
from tmp_holding h
join ndb n on h.nav_ident = n.ident and abs(n.lonx - h.lonx) < 0.00001 and abs(n.laty - h.laty) < 0.00001;

-- Set airport_id ==================
update holding set airport_id = (
  select a.airport_id
  from airport a
  where holding.airport_ident is not null and holding.airport_ident = a.ident
) where airport_id is null;

-- course is now magnetic and will be corrected later in DfdCompiler::updateMagvar()
