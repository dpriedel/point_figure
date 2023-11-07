-- this function will generate a series of rows for the
-- Baltic Dry Index, which is in its own table,
-- with date, close, x-day moving avg and x-day standard deviation.
-- This data can be used to plot daily closing stock prices
-- with bollinger bands.
-- 
DROP FUNCTION IF EXISTS BDI_DAILY_CLOSING_DATA_WITH_STDDEV(DATE, DATE, INT);

CREATE OR REPLACE FUNCTION BDI_DAILY_CLOSING_DATA_WITH_STDDEV(IN DATE, IN DATE, IN INT) -- noqa: disable=L016
RETURNS TABLE( -- noqa: disable=L003
    date DATE,
    close_p NUMERIC(12, 4),
    moving_avg NUMERIC(12, 4),
    std_dev NUMERIC(12, 4)
)
AS
$BODY$

DECLARE
period INT := $3 - 1;
start_date DATE := $1 - 2 * period;
end_date DATE := $2;

BEGIN

    RETURN QUERY
    SELECT q1.date, q1.close_p, CAST(q1.moving_avg AS NUMERIC(12, 4)),
    CAST(q1.std_dev AS NUMERIC(12, 4)) FROM
    (SELECT bdi.date, bdi.close_p,
        AVG(bdi.close_p) OVER (ORDER BY bdi.date ASC 
            ROWS BETWEEN period PRECEDING AND CURRENT ROW) AS moving_avg,
        STDDEV_POP(bdi.close_p) OVER (ORDER BY bdi.date ASC 
            ROWS BETWEEN period PRECEDING AND CURRENT ROW) AS std_dev
        FROM new_stock_data.bdi
        WHERE bdi.date BETWEEN start_date AND end_date
        ORDER BY bdi.date ASC) AS q1
    WHERE q1.date >= $1
    ;
    RETURN;
END;
$BODY$

LANGUAGE 'plpgsql' STABLE
COST 100
ROWS 200;

ALTER FUNCTION BDI_DAILY_CLOSING_DATA_WITH_STDDEV(DATE, DATE, INT)
OWNER TO data_updater_pg;
