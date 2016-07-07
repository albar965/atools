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
-- This script create all metadata tables
-- *************************************************************

drop table if exists metadata;

-- Metadata that identifies the schema version and allows to check for compatibility with an application
create table metadata
(
  db_version_major integer not null,  -- Major version. The schema is considered incompatible if this differs
                                      -- from application supported version (i.e. can cause SQL errors, crashes, etc.)
  db_version_minor integer not null,  -- Minor version. Only for updates that do not break compatibility.
  last_load_timestamp varchar(100)    -- Timestamp of last loading (i.e. "2016-07-05T15:45:30.396")
);

-- **************************************************

drop table if exists scenery_area;

-- An area entry from the scenery.cfg file.
-- See https://msdn.microsoft.com/en-us/library/cc526966.aspx for more information about this file
create table scenery_area
(
  scenery_area_id integer primary key,
  number integer not null,             -- The number part of [Area.001]
  layer integer not null,              -- Layer number
  title varchar(250) not null,         -- Area title as shown in the library in FS
  remote_path varchar(250),            -- Unused
  local_path varchar(250) not null,    -- Scenery path relative to FS base directory or an absolute path
  active integer not null,             -- Boolean - 1 if active
  required integer not null,           -- Boolean - 1 if this entry cannot be deleted
  exclude varchar(50)
);

-- **************************************************

drop table if exists bgl_file;

-- An entry for each BGL file that was read and contained airports or navaids
-- Orphaned files can occur if an airport was deleted later by an add-on
create table bgl_file
(
  bgl_file_id integer primary key,
  scenery_area_id integer not null,
  bgl_create_time integer not null,        -- Creation time that was stored in the BGL. Seconds since Epoch.
  file_modification_time integer not null, -- Modification time of the file. Seconds since Epoch.
  filepath varchar(1000) not null,         -- Absolute filename including full path
  filename varchar(250) not null,          -- Filename only - redundant and used for search functionality
  size integer not null,                   -- File size in bytes
foreign key(scenery_area_id) references scenery_area(scenery_area_id)
);

create index if not exists idx_bgl_file_scenery_area_id on bgl_file(scenery_area_id);

