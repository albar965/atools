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

-- *******************************************************************************
-- This script creates the logbook undo table which is kept in its own database
-- *******************************************************************************


-- Tables needed for undo support in logbook table ===============================================

drop table if exists undo_current;

create table undo_current
(
  undo_group_id integer primary key
);

-- Insert dummy value
-- Undo id points to the step which will be reverted when calling undo(). Can be min(undo_group_id) - 1 to max(undo_group_id)
insert into undo_current (undo_group_id) values(0);

-- ===========================================================================
drop table if exists undo_data;

CREATE TABLE undo_data
(
  undo_data_id integer primary key,
  undo_group_id integer not null,        -- Id for one undo changeset covering one or more updates, deletions or inserts
  undo_type varchar(1) not null,         -- U for update, I for insert and D for deletion

  -- Copy of all columns from table "logbook"
  logbook_id integer not null,
  aircraft_name varchar(250) collate nocase,
  aircraft_type varchar(250) collate nocase,
  aircraft_registration varchar(50) collate nocase,
  flightplan_number varchar(100) collate nocase,
  flightplan_cruise_altitude double,
  flightplan_file varchar(1024) collate nocase,
  performance_file varchar(1024) collate nocase,
  block_fuel double,
  trip_fuel double,
  used_fuel double,
  is_jetfuel integer,
  grossweight double,
  distance double,
  distance_flown double,
  departure_ident varchar(10) collate nocase,
  departure_name varchar(200) collate nocase,
  departure_runway varchar(10) collate nocase,
  departure_lonx double,
  departure_laty double,
  departure_alt double,
  departure_time varchar(100),
  departure_time_sim varchar(100),
  destination_ident varchar(10) collate nocase,
  destination_name varchar(200) collate nocase,
  destination_runway varchar(10) collate nocase,
  destination_lonx double,
  destination_laty double,
  destination_alt  double,
  destination_time varchar(100),
  destination_time_sim varchar(100),
  route_string varchar(1024         ),
  simulator varchar(50) collate nocase,
  description varchar(2048) collate nocase,
  flightplan blob,
  aircraft_perf blob,
  aircraft_trail blob
);

create index if not exists  idx_undo_data_id on undo_data(undo_data_id);
create index if not exists  idx_undo_group_id on undo_data(undo_group_id);
create index if not exists  idx_undo_logbook_id on undo_data(logbook_id);
