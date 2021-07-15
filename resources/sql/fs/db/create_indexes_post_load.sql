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

-- *************************************************************
-- This script create all indexes after loading the data.
-- The indexes are needed for post process steps
-- *************************************************************

create index if not exists idx_airport_icao on airport(icao);
create index if not exists idx_airport_iata on airport(iata);
create index if not exists idx_airport_local on airport(faa);
create index if not exists idx_airport_local on airport(local);
create index if not exists idx_airport_xpident on airport(xpident);

create index if not exists idx_airport_file_ident on airport_file(ident);
create index if not exists idx_airport_file_file_id on airport_file(file_id);

create index if not exists idx_waypoint_type on waypoint(type);
create index if not exists idx_waypoint_region on waypoint(region);
create index if not exists idx_waypoint_ident on waypoint(ident);
create index if not exists idx_waypoint_num_vairway on waypoint(num_victor_airway);
create index if not exists idx_waypoint_num_rairway on waypoint(num_jet_airway);
create index if not exists idx_waypoint_lonx on waypoint(lonx);
create index if not exists idx_waypoint_laty on waypoint(laty);

create index if not exists idx_runway_end_ils_ident on runway_end(ils_ident);

create index if not exists idx_vor_ident on vor(ident);
create index if not exists idx_vor_type on vor(type);
create index if not exists idx_vor_region on vor(region);
create index if not exists idx_vor_lonx on vor(lonx);
create index if not exists idx_vor_laty on vor(laty);

create index if not exists idx_ndb_ident on ndb(ident);
create index if not exists idx_ndb_type on ndb(type);
create index if not exists idx_ndb_region on ndb(region);
create index if not exists idx_ndb_lonx on ndb(lonx);
create index if not exists idx_ndb_laty on ndb(laty);
