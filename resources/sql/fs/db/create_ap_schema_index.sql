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
-- This script create all airport related indexes
-- *************************************************************


create index if not exists idx_airport_file_id on airport(file_id);
create index if not exists idx_airport_ident on airport(ident);
create index if not exists idx_com_airport_id on com(airport_id);
create index if not exists idx_helipad_airport_id on helipad(helipad_id);
create index if not exists idx_start_airport_id on start(airport_id);
create index if not exists idx_start_runway_end_id on start(runway_end_id);
create index if not exists idx_apron_airport_id on apron(airport_id);
create index if not exists idx_taxi_path_airport_id on taxi_path(airport_id);
create index if not exists idx_runway_airport_id on runway(airport_id);
create index if not exists idx_runway_primary_end_id on runway(primary_end_id);
create index if not exists idx_runway_secondary_end_id on runway(secondary_end_id);
create index if not exists idx_runway_end_name on runway_end(name);
create index if not exists idx_approach_airport_id on approach(airport_id);
create index if not exists idx_approach_runway_end_id on approach(runway_end_id);
create index if not exists idx_approach_airport_ident on approach(airport_ident);
create index if not exists idx_approach_runway_name on approach(runway_name);
create index if not exists idx_transition_approach_id on transition(approach_id);
create index if not exists idx_approach_leg_approach_id on approach_leg(approach_id);
create index if not exists idx_transition_leg_transition_id on transition_leg(transition_id);
create index if not exists idx_parking_airport_id on parking(airport_id);

