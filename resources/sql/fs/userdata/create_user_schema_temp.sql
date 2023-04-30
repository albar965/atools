-- *****************************************************************************
-- Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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
-- This script creates the user defined waypoint table which is kept in its own database
-- The temporary table is only used for cleanup
-- *******************************************************************************

drop table if exists tempuserdata;

create table tempuserdata
(
  userdata_id integer primary key,
  type varchar(10) collate nocase,
  name varchar(200) collate nocase,
  ident varchar(10) collate nocase,
  region varchar(10) collate nocase,
  description varchar(1024) collate nocase,
  tags varchar(1024) collate nocase,
  lonx double not null,
  laty double not null
);

create index if not exists idx_tempuserdata_lonx on tempuserdata(lonx);
create index if not exists idx_tempuserdata_laty on tempuserdata(laty);
