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

-- *******************************************************************************
-- This script creates the userpoint/userdata table which is kept in its own database
-- *******************************************************************************

drop table if exists userdata;

-- VRP,1NM NORTH SALERNO TOWN,1NSAL,40.6964,14.785,0,0,IT,FROM SOR VOR: 069° 22NM
create table userdata
(
  userdata_id integer primary key,
  type varchar(10) collate nocase,           -- VRP, POI, OBS, IFR, etc.
  name varchar(200) collate nocase,
  ident varchar(10) collate nocase,          -- Maximum five characters for export
  region varchar(10) collate nocase,          -- Max two characters for export
  description varchar(1024) collate nocase,
  tags varchar(1024) collate nocase,         -- Simple text file
  last_edit_timestamp varchar(100) not null, -- Timestamp of last edit (i.e. "2016-07-05T15:45:30.396")
  import_file_path varchar(512),             -- Full path filename from import
  visible_from integer,                      -- Visible from zoom NM
  altitude integer,                          -- ft
  temp integer,                         -- Entry will be deleted on startup if 1
  lonx double not null,
  laty double not null
);

create index if not exists idx_userdata_type on userdata(type);
create index if not exists idx_userdata_name on userdata(name);
create index if not exists idx_userdata_ident on userdata(ident);
create index if not exists idx_userdata_region on userdata(region);
create index if not exists idx_userdata_description on userdata(description);
create index if not exists idx_userdata_tags on userdata(tags);
create index if not exists idx_userdata_temp on userdata(temp);
create index if not exists idx_userdata_last_edit_timestamp on userdata(last_edit_timestamp);
create index if not exists idx_userdata_import_file_path on userdata(import_file_path);
create index if not exists idx_userdata_lonx on userdata(lonx);
create index if not exists idx_userdata_laty on userdata(laty);
