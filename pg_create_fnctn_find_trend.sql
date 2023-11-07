-- this function will generate a series of rows for symbols
-- which have 4 days of increasing closing price.
--

DROP FUNCTION IF EXISTS new_stock_data.find_trend(INT, DATE, NUMERIC, BIGINT);

CREATE OR REPLACE FUNCTION new_stock_data.find_trend(
    IN trend_len INT DEFAULT 4,
    IN start_date DATE DEFAULT current_date - 20,
    IN min_close NUMERIC(14, 4) DEFAULT 10.0,
    IN min_volume BIGINT DEFAULT 100000
)
RETURNS TABLE (
    symbol TEXT,
    date DATE,
    adjclose NUMERIC(14, 4)
)
AS
$BODY$

-- DECLARE
-- 	period INT := $4 - 1;

BEGIN
    RETURN QUERY
    SELECT
        t1.symbol,
        t1.date,
        t1.adjclose
    FROM
        (
        SELECT
            t0.date,
            t0.symbol,
            t0.adjclose,
            SUM(t0.close_up_or_down) OVER w AS trend
        FROM
            new_stock_data.current_data AS t0
        WHERE
            t0.date > start_date
            AND t0.adjclose >= min_close
            AND t0.adjvolume >= min_volume
        WINDOW w AS (PARTITION BY t0.symbol ORDER BY t0.date ASC ROWS BETWEEN trend_len - 1 PRECEDING AND CURRENT ROW)
        ORDER BY
            t0.symbol, t0.date
        ) AS t1
    WHERE
        t1.trend >= trend_len;
    RETURN;
END;
$BODY$

LANGUAGE 'plpgsql' STABLE
COST 100
ROWS 2000;

ALTER FUNCTION new_stock_data.find_trend(INT, DATE, NUMERIC, BIGINT) OWNER TO data_updater_pg;
