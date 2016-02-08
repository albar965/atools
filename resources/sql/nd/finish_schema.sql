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


-- Create indexes on columns that are potentially used in searches

create index if not exists idx_airport_name on airport(name);
create index if not exists idx_airport_country on airport(country);
create index if not exists idx_airport_state on airport(state);
create index if not exists idx_airport_city on airport(city);

create index if not exists idx_airport_has_avgas on airport(has_avgas);
create index if not exists idx_airport_has_jetfuel on airport(has_jetfuel);
create index if not exists idx_airport_has_tower on airport(has_tower);

create index if not exists idx_airport_is_closed on airport(is_closed);
create index if not exists idx_airport_is_military on airport(is_military);

create index if not exists idx_airport_num_boundary_fence on airport(num_boundary_fence);
create index if not exists idx_airport_num_com on airport(num_com);
create index if not exists idx_airport_num_parking_gate on airport(num_parking_gate);
create index if not exists idx_airport_num_parking_ga_ramp on airport(num_parking_ga_ramp);
create index if not exists idx_airport_num_approach on airport(num_approach);

create index if not exists idx_airport_num_runway_hard on airport(num_runway_hard);
create index if not exists idx_airport_num_runway_soft on airport(num_runway_soft);
create index if not exists idx_airport_num_runway_water on airport(num_runway_water);
create index if not exists idx_airport_num_runway_light on airport(num_runway_light);

create index if not exists idx_airport_num_runway_end_closed on airport(num_runway_end_closed);
create index if not exists idx_airport_num_runway_end_vasi on airport(num_runway_end_vasi);
create index if not exists idx_airport_num_runway_end_als on airport(num_runway_end_als);
create index if not exists idx_airport_num_runway_end_ils on airport(num_runway_end_ils);

create index if not exists idx_airport_num_apron on airport(num_apron);
create index if not exists idx_airport_num_taxi_path on airport(num_taxi_path);
create index if not exists idx_airport_num_helipad on airport(num_helipad);
create index if not exists idx_airport_num_jetway on airport(num_jetway);
create index if not exists idx_airport_longest_runway_length on airport(longest_runway_length);
create index if not exists idx_airport_longest_runway_width on airport(longest_runway_width);
create index if not exists idx_airport_longest_runway_heading on airport(longest_runway_heading);
create index if not exists idx_airport_longest_runway_surface on airport(longest_runway_surface);
create index if not exists idx_airport_num_runways on airport(num_runways);
create index if not exists idx_airport_largest_parking_ramp on airport(largest_parking_ramp);
create index if not exists idx_airport_largest_parking_gate on airport(largest_parking_gate);
create index if not exists idx_airport_rating on airport(rating);

create index if not exists idx_airport_left_lonx on airport(left_lonx);
create index if not exists idx_airport_top_laty on airport(top_laty);
create index if not exists idx_airport_right_lonx on airport(right_lonx);
create index if not exists idx_airport_bottom_laty on airport(bottom_laty);

create index if not exists idx_airport_altitude on airport(altitude);
create index if not exists idx_airport_lonx on airport(lonx);
create index if not exists idx_airport_laty on airport(laty);
