-- *****************************************************************************
-- Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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
-- Remove any duplicates that are resulting from add-on BGL files.
-- Some add-on airport BGL files contain additional navaids that are
-- not (and cannot be) covered by the delete processor. These will
-- be removed here by keeping only the duplicate with the highest id.
-- This means stock/default/oldest are removed and add-on are kept.
-- The manhattan distance using deg is sufficient for a crude distance estimation
-- *************************************************************


-- Delete duplicate ils same name and same type
delete from ils where ils_id in (
select distinct w1.ils_id
from ils w1
join ils w2 on w1.ident = w2.ident and w1.name = w2.name
where
w1.ils_id < w2.ils_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.1);

-- Delete duplicate ils same name and close by
delete from ils where ils_id in (
select distinct w1.ils_id
from ils w1
join ils w2 on w1.ident = w2.ident
where
w1.ils_id < w2.ils_id and
(abs(w1.lonx - w2.lonx) + abs(w1.laty - w2.laty)) < 0.01);

