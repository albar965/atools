-- *****************************************************************************
-- Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
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
-- This script create all boundary and MORA tables
-- *************************************************************

drop table if exists boundary;

-- Airspace boundary
create table boundary
(
  boundary_id integer primary key,
  file_id integer not null,
  type varchar(15),                     -- see enum atools::fs::bgl::boundary::BoundaryType
  name varchar(250),
  restrictive_designation varchar(20),  -- Number or name to display together with type on the map only for restricted airspaces
  restrictive_type varchar(20),         -- Single character for restricted type only
  multiple_code varchar(5),             -- Airspace having the same designator but subdivided or differently divided by lateral and/or vertical detail
  time_code varchar(5),                 -- C active continuously, including holidays
                                        -- H active continuously, excluding holidays
                                        -- N active not continuously - time not known
                                        -- NULL active times announced by Notams
                                        -- U Unknown - do not display value
  com_type varchar(30),                 -- If airspace has a COM frequency. see enum atools::fs::bgl::com::ComType
  com_frequency integer,                -- frequency in MHz * 1000
  com_name varchar(50),
  min_altitude_type varchar(15),        -- see enum atools::fs::bgl::boundary::AltitudeType - null if unknown
  max_altitude_type varchar(15),        -- "
  min_altitude integer,                 -- Lower altitude for this airspace in feet
  max_altitude integer,                 -- Upper altitude for this airspace in feet
  max_lonx double not null,             -- Bounding rectangle
  max_laty double not null,             -- "
  min_lonx double not null,             -- Bounding rectangle
  min_laty double not null,             -- "
  geometry blob,                        -- Pre calculated geometry
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_boundary_file_id on boundary(file_id);

-- minimum off route altitude
drop table if exists mora_grid;

-- MORA grid
create table mora_grid
(
  mora_grid_id integer primary key,
  version integer not null,
  lonx_columns integer not null,
  laty_rows integer not null,
  geometry blob not null
);
