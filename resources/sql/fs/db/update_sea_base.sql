-- *****************************************************************************
-- Copyright 2015-2019 Gérard B. alias gaab
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

-- This script removes fake runways that were created by add-on authors for AI traffic only.
-- FSX/P3D

-- 1 Select aeroport with water runways, concrete runways width quasi null runways, addon as neither FSX nor P3D are using this trick

create table suspect as
select airport.airport_id, airport.ident, runway.runway_id, runway.surface, runway.width, runway.length, runway.heading, runway.laty, runway.lonx
  from (airport inner join runway on airport.airport_id=runway.airport_id)
    where airport.num_runway_water > 0 and airport.num_runway_hard > 0
  and ((runway.surface = 'C' and runway.width < 2) or runway.surface = 'W')
  and airport.is_addon = 1
  order by airport.ident;


-- 2 Find fake concrete runways :
--       criterias almost same heading (or reverse heading..)
--       very near centers

--    May be should be more strict on heading difference (20?) and position distance (0.04) ...
--    but allow to get most of the fake runway, even at a risk (verified)

create table fake as
  Select C.airport_id AS 'CAirport', C.ident AS 'Icao',
    C.runway_id as 'CRunway', C.surface as 'CSurface', C.width as 'CWidth', C.length as 'CLength',
    C.heading as 'CHeading', C.laty as 'CLaty', C.lonx as 'CLongx',
    W.runway_id as 'WRunway', W.surface as 'WSurface', W.width as 'WWidth', W.length as 'WLength',
    W.heading as 'WHeading', W.laty as 'WLaty', W.lonx as 'WLongx'
    from suspect C
    inner join suspect W on W.airport_id = C.airport_id
    where C.surface = 'C' and W.surface = 'W'
    and (abs(C.heading - W.heading) < 20 or abs(abs(C.heading - W.heading) - 180) < 20)
    and abs(C.laty - W.laty) < 0.04
    and abs(C.lonx - W.lonx) < 0.04
    order by C.ident;


-- 3 Update airport and suppress fake concrete runways
update airport set num_runway_hard = num_runway_hard -
  (select count(CRunway) from fake where airport.airport_id = fake.CAirport group by CAirport)
  where airport_id in
    (select CAirport from fake);

--update airport_large set num_runway_hard = num_runway_hard -
--  (select count(CRunway) from fake where airport_large.airport_id = fake.CAirport group by CAirport)
--  where airport_id in
--    (select CAirport from fake);
--update airport_medium set num_runway_hard = num_runway_hard -
--  (select count(CRunway) from fake where airport_medium.airport_id = fake.CAirport group by CAirport)
--  where airport_id in
--    (select CAirport from fake);

delete from runway
  where runway_id in
    (select CRunway from fake);

-- List number of deleted runways
select count(1) from fake;

drop table suspect;
drop table fake;

