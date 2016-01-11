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

-- Drop all indexes to speed up loading

drop index if exists idx_logbook_startdate;
drop index if exists idx_logbook_airport_from_icao;
drop index if exists idx_logbook_airport_to_icao;
drop index if exists idx_logbook_description;
drop index if exists idx_logbook_total_time;
drop index if exists idx_logbook_night_time;
drop index if exists idx_logbook_instrument_time;
drop index if exists idx_logbook_aircraft_reg;
drop index if exists idx_logbook_aircraft_descr;
drop index if exists idx_logbook_aircraft_type;
drop index if exists idx_logbook_aircraft_flags;

drop index if exists idx_logbook_visits_lbd;
drop index if exists idx_logbook_visits_ap;
