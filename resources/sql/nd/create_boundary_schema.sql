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

drop table if exists boundary;

create table boundary
(
  boundary_id integer primary key,
  file_id integer not null,
  type varchar(15),
  name varchar(30),
  com_type varchar(30),
  com_frequency integer,
  com_name varchar(50),
  min_altitude_type varchar(15),
  max_altitude_type varchar(15),
  max_altitude integer not null,
  max_lonx double not null,
  max_laty double not null,
  min_altitude integer not null,
  min_lonx double not null,
  min_laty double not null,
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_boundary_file_id on boundary(file_id);


-- **************************************************

drop table if exists boundary_line;

create table boundary_line
(
  boundary_line_id integer primary key,
  boundary_id integer not null,
  type varchar(15),
  radius double,
  lonx double,
  laty double,
foreign key(boundary_id) references boundary(boundary_id)
);

create index if not exists idx_boundary_line_boundary_id on boundary_line(boundary_id);

