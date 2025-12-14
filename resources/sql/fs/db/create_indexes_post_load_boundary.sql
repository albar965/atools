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

-- *************************************************************
-- This script create all indexes after loading the data for the boundary table.
-- The indexes are needed for post process steps
-- *************************************************************

create index if not exists idx_boundary_type on boundary(type);
create index if not exists idx_boundary_name on boundary(name);
create index if not exists idx_boundary_max_altitude on boundary(max_altitude);
create index if not exists idx_boundary_max_lonx on boundary(max_lonx);
create index if not exists idx_boundary_max_laty on boundary(max_laty);
create index if not exists idx_boundary_min_altitude on boundary(min_altitude);
create index if not exists idx_boundary_min_lonx on boundary(min_lonx);
create index if not exists idx_boundary_min_laty on boundary(min_laty);
