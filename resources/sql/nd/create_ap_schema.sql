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

drop table if exists airport;

create table airport
(
  airport_id integer primary key,
  file_id integer not null,
  ident text not null,
  region text,
  name text,
  country text not null,
  state text,
  city text not null,
  fuel_flags integer not null,
  has_avgas integer not null,
  has_jetfuel integer not null,
  has_boundary_fence integer not null,
  has_tower_object integer not null,
  has_taxiways integer not null,
  has_jetways integer not null,
  mag_var real not null,
  tower_lonx real,
  tower_laty real,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_airport_file_id on airport(file_id);
create index if not exists idx_airport_ident on airport(ident);

-- **************************************************

drop table if exists com;

create table com
(
  com_id integer primary key,
  airport_id not null,
  type text not null,
  frequency integer not null,
  name text,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_com_airport_id on com(airport_id);

-- **************************************************

drop table if exists helipad;

create table helipad
(
  helipad_id integer primary key,
  airport_id not null,
  surface text not null,
  type text not null,
  length real not null,
  width real not null,
  heading real not null,
  is_transparent integer not null,
  is_closed integer not null,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_helipad_airport_id on helipad(helipad_id);

-- **************************************************

drop table if exists start;

create table start
(
  start_id integer primary key,
  airport_id not null,
  runway_end_id integer,
  type text not null,
  heading real not null,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(airport_id) references airport(airport_id),
foreign key(runway_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_start_airport_id on start(airport_id);
create index if not exists idx_start_runway_end_id on start(runway_end_id);

-- **************************************************

drop table if exists apron;

create table apron
(
  apron_id integer primary key,
  airport_id not null,
  surface text not null,
  is_draw_surface integer not null,
  is_detail integer not null,
  vertices text not null,
  vertices2 text not null,
  triangles text not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_apron_airport_id on apron(airport_id);

-- **************************************************

drop table if exists apron_light;

create table apron_light
(
  apron_light_id integer primary key,
  airport_id not null,
  vertices text not null,
  edges text not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_apron_light_airport_id on apron_light(airport_id);

-- **************************************************

drop table if exists runway;

create table runway
(
  runway_id integer primary key,
  airport_id not null,
  primary_end_id integer not null,
  secondary_end_id integer  not null,
  surface text not null,
  length integer not null,
  width integer not null,
  heading real not null,
  pattern_altitude integer not null,
  marking_flags integer not null,
  light_flags integer not null,
  pattern_flags integer not null,
  edge_light text,
  center_light text,
  has_center_red integer not null,
  altitude integer not null,
  lonx real not null,
  laty real not null,
foreign key(airport_id) references airport(airport_id),
foreign key(primary_end_id) references runway_end(runway_end_id),
foreign key(secondary_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_runway_airport_id on runway(airport_id);
create index if not exists idx_runway_primary_end_id on runway(primary_end_id);
create index if not exists idx_runway_secondary_end_id on runway(secondary_end_id);

-- **************************************************

drop table if exists runway_end;

create table runway_end
(
  runway_end_id integer primary key,
  name text not null,
  offsetThreshold integer not null,
  blastPad integer not null,
  overrun integer not null,
  left_vasi_type text,
  left_vasi_pitch real,
  right_vasi_type text,
  right_vasi_pitch real,
  has_closed_markings integer not null,
  has_stol_markings integer not null,
  is_takeoff integer not null,
  is_landing integer not null,
  is_pattern text not null,
  app_light_system_type text,
  has_end_lights integer not null,
  has_reils integer not null,
  has_touchdown_lights integer not null,
  num_strobes integer not null
);

create index if not exists idx_runway_end_name on runway_end(name);

-- **************************************************

drop table if exists approach;

create table approach
(
  approach_id integer primary key,
  runway_end_id integer,
  type text not null,
  has_gps_overlay integer not null,
  num_legs integer not null,
  num_missed_legs integer not null,
  fix_type text not null,
  fix_ident text,
  fix_region text,
  fix_airport_ident text,
  altitude integer,
  heading real,
  missed_altitude integer,
foreign key(runway_end_id) references runway_end(runway_end_id)
);

create index if not exists idx_approach_runway_end_id on approach(runway_end_id);

-- **************************************************

drop table if exists transition;

create table transition
(
  transition_id integer primary key,
  approach_id integer not null,
  type text not null,
  num_legs integer not null,
  fix_type text not null,
  fix_ident text,
  fix_region text,
  fix_airport_ident text,
  altitude integer,
  dme_ident text,
  dme_region text,
  dme_airport_ident,
  dme_radial real,
  dme_distance integer,
foreign key(approach_id) references approach(approach_id)
);

create index if not exists idx_transition_approach_id on transition(approach_id);

-- **************************************************

drop table if exists parking;

create table parking
(
  parking_id integer primary key,
  airport_id integer not null,
  type text not null,
  name text not null,
  number integer not null,
  radius real not null,
  heading real not null,
  lonx real not null,
  laty real not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_parking_airport_id on parking(airport_id);

-- **************************************************

drop table if exists delete_airport;

create table delete_airport
(
  delete_airport_id integer primary key,
  airport_id integer not null,
  num_del_runway integer not null,
  num_del_start integer not null,
  num_del_com integer not null,
  approaches integer not null,
  apronlights integer not null,
  aprons  integer not null,
  frequencies  integer not null,
  helipads  integer not null,
  runways  integer not null,
  starts  integer not null,
  taxiways  integer not null,
foreign key(airport_id) references airport(airport_id)
);

create index if not exists idx_delete_airport_airport_id on delete_airport(airport_id);
