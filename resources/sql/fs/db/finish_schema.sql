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
-- Create application indexes after all post processes
-- *************************************************************

-- Create indexes which are used in searches
create index if not exists idx_ils_ident on ils(ident);
create index if not exists idx_ils_type on ils(type);

create index if not exists idx_airway_left_lonx on airway(left_lonx);
create index if not exists idx_airway_top_laty on airway(top_laty);
create index if not exists idx_airway_right_lonx on airway(right_lonx);
create index if not exists idx_airway_bottom_laty on airway(bottom_laty);

create index if not exists idx_airway_from_lonx on airway(from_lonx);
create index if not exists idx_airway_from_laty on airway(from_laty);
create index if not exists idx_airway_to_lonx on airway(to_lonx);
create index if not exists idx_airway_to_laty on airway(to_laty);
create index if not exists idx_airway_name on airway(airway_name);

-- Airport MSA indexes ========================================================
create index if not exists idx_airport_msa_airport_id on airport_msa(airport_id);
create index if not exists idx_airport_msa_nav_id on airport_msa(nav_id);

create index if not exists idx_airport_msa_left_lonx on airport_msa(left_lonx);
create index if not exists idx_airport_msa_top_laty on airport_msa(top_laty);
create index if not exists idx_airport_msa_right_lonx on airport_msa(right_lonx);
create index if not exists idx_airport_msa_bottom_laty on airport_msa(bottom_laty);
create index if not exists idx_airport_msa_lonx on airport_msa(lonx);
create index if not exists idx_airport_msa_laty on airport_msa(laty);


-- Enroute holding indexes ========================================================
create index if not exists idx_holding_airport_id on holding(airport_id);
create index if not exists idx_holding_nav_id on holding(nav_id);
create index if not exists idx_holding_lonx on holding(lonx);
create index if not exists idx_holding_laty on holding(laty);

-- Collect table and index statistics
-- Disabled for now since it can cause queries to freeze
-- analyze;
