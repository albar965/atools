-- *****************************************************************************
-- Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
-- Views for debugging purposes
-- *************************************************************

drop view if exists v_airport_scenery;

create view v_airport_scenery as
select a.airport_id, a.ident, a.name, a.country, a.state, a.city,
  f.filename, s.layer as scenery_layer, s.title as scenery_title
from airport a join bgl_file f on a.file_id = f.bgl_file_id
join scenery_area s on s.scenery_area_id = f.scenery_area_id
order by a.airport_id;

drop view if exists v_airport_runway;

create view v_airport_runway as
select airport_id, ident, name, country, state, city, length, width, surface,
runway_id, primary_end_id, secondary_end_id, runway_name, ils_ident, ils_name
from (
select a.airport_id, a.ident, a.name, a.country, a.state, a.city, r.length, r.width, r.surface,
r.runway_id, r.primary_end_id, r.secondary_end_id, pe.name as runway_name, pi.ident as ils_ident, pi.name as ils_name
from airport a join runway r on a.airport_id = r.airport_id
join runway_end pe on r.primary_end_id = pe.runway_end_id
left outer join ils pi on pe.runway_end_id = pi.loc_runway_end_id
union all
select a.airport_id, a.ident, a.name, a.country, a.state, a.city, r.length, r.width, r.surface,
r.runway_id, r.primary_end_id, r.secondary_end_id, se.name as runway_name, si.ident as ils_ident, si.name as ils_name
from airport a join runway r on a.airport_id = r.airport_id
join runway_end se on r.secondary_end_id = se.runway_end_id
left outer join ils si on se.runway_end_id = si.loc_runway_end_id)
order by airport_id, runway_name;

drop view if exists v_airport_com;

create view v_airport_com as
select a.ident, a.name, a.country, a.state, a.city, c.name as com_name, c.type as com_type,
  printf("%3.3f", cast(c.frequency as double) / 1000) as com_frequency
from airport a join com c on a.airport_id = c.airport_id
order by a.airport_id;


drop view if exists v_airport_approach;

create view v_airport_approach as
select airport_id, ident, airport_name, country, state, city, runway_end_id,
runway_name, approach_id, approach_type, transition_type
from (
select a.airport_id, a.ident, a.name as airport_name, a.country, a.state, a.city, pe.runway_end_id,
pe.name as runway_name, pa.approach_id, pa.type as approach_type, pt.fix_type as transition_type
from airport a join runway r on a.airport_id = r.airport_id
join runway_end pe on r.primary_end_id = pe.runway_end_id
join approach pa on pa.runway_end_id = pe.runway_end_id
left outer join transition pt on pa.approach_id = pt.approach_id
union all
select a.airport_id, a.ident, a.name as airport_name, a.country, a.state, a.city, se.runway_end_id,
se.name as runway_name, sa.approach_id, sa.type as approach_type, st.fix_type as transition_type
from airport a join runway r on a.airport_id = r.airport_id
join runway_end se on r.secondary_end_id = se.runway_end_id
join approach sa on sa.runway_end_id = se.runway_end_id
left outer join transition st on sa.approach_id = st.approach_id)
order by airport_id, runway_name;


drop view if exists v_airport_delete;

create view v_airport_delete as
select airport_id, ident, name, country, state, city,
  approaches, apronlights, frequencies, helipads, runways, starts, taxiways, filename, layer
from (
select a.airport_id, a.ident, a.name, a.country, a.state, a.city, d.approaches, d.apronlights, d.frequencies, d.helipads, d.runways, d.starts, d.taxiways, f.filename, a.layer
from airport a join delete_airport d on a.airport_id = d.airport_id
join bgl_file f on a.file_id = f.bgl_file_id
join scenery_area a on f.scenery_area_id = a.scenery_area_id
union all
select o.airport_id, o.ident, o.name, o.country, o.state, o.city, null as approaches, null as apronlights, null as frequencies, null as helipads, null as runways, null as starts, null as taxiways, f.filename, a.layer
from airport o join bgl_file f on o.file_id = f.bgl_file_id
join scenery_area a on f.scenery_area_id = a.scenery_area_id
where o.airport_id not in (select airport_id from delete_airport) and
o.ident in (select ident from delete_airport join airport on delete_airport.airport_id = airport.airport_id)
) order by airport_id, layer;

