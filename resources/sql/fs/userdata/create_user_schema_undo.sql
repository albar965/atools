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


-- Tables needed for undo support in userpoint table ===============================================

-- ===========================================================================
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

  -- Copy of all columns from table "userdata"
  userdata_id integer not null,
  type varchar(10) collate nocase,
  name varchar(200) collate nocase,
  ident varchar(10) collate nocase,
  region varchar(10) collate nocase,
  description varchar(1024) collate nocase,
  tags varchar(1024) collate nocase,
  last_edit_timestamp varchar(100) not null,
  import_file_path varchar(512),
  visible_from integer,
  altitude integer,
  temp integer,
  lonx double not null,
  laty double not null
);

create index if not exists  idx_undo_data_id on undo_data(undo_data_id);
create index if not exists  idx_undo_group_id on undo_data(undo_group_id);
create index if not exists  idx_undo_userdata_id on undo_data(userdata_id);
