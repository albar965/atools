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

drop table if exists scenery_area;

create table scenery_area
(
  scenery_area_id integer primary key,
  number integer not null,
  layer integer not null,
  title varchar(250) not null,
  remote_path varchar(250),
  local_path varchar(250) not null,
  active integer not null,
  required integer not null,
  exclude varchar(50)
);

-- **************************************************

drop table if exists bgl_file;

create table bgl_file
(
  bgl_file_id integer primary key,
  scenery_area_id integer not null,
  bgl_create_time integer not null,
  file_modification_time integer not null,
  filepath varchar(1000) not null,
  filename varchar(250) not null,
  size integer not null,
foreign key(scenery_area_id) references scenery_area(scenery_area_id)
);

create index if not exists idx_bgl_file_scenery_area_id on bgl_file(scenery_area_id);

