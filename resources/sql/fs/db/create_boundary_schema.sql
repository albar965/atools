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

-- *************************************************************
-- This script create all boundary tables
-- *************************************************************

drop table if exists boundary;

-- Airspace boundary
create table boundary
(
  boundary_id integer primary key,
  file_id integer not null,
  type varchar(15),               -- see enum atools::fs::bgl::boundary::BoundaryType
  name varchar(250),
  com_type varchar(30),           -- If airspace has a COM frequency. see enum atools::fs::bgl::com::ComType
  com_frequency integer,          -- frequency in MHz * 1000
  com_name varchar(50),
  min_altitude_type varchar(15),  -- see enum atools::fs::bgl::boundary::AltitudeType - null if unknown
  max_altitude_type varchar(15),  -- "
  min_altitude integer,           -- Lower altitude for this airspace in feet
  max_altitude integer,           -- Upper altitude for this airspace in feet
  max_lonx double not null,       -- Bounding rectange
  max_laty double not null,       -- "
  min_lonx double not null,       -- Bounding rectange
  min_laty double not null,       -- "
  geometry blob,                  -- Pre calculated geometry
foreign key(file_id) references bgl_file(bgl_file_id)
);

create index if not exists idx_boundary_file_id on boundary(file_id);
