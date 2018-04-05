-- *****************************************************************************
-- Copyright 2015-2018 Alexander Barthel albar965@mailbox.org
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


drop table if exists userdata;

drop index if exists idx_userdata_type;
drop index if exists idx_userdata_name;
drop index if exists idx_userdata_ident;
drop index if exists idx_userdata_region;
drop index if exists idx_userdata_description;
drop index if exists idx_userdata_tags;
drop index if exists idx_userdata_temp;
drop index if exists idx_userdata_last_edit_timestamp;
drop index if exists idx_userdata_import_file_path;
drop index if exists idx_userdata_lonx;
drop index if exists idx_userdata_laty;
