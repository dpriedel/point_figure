-- this function will generate a series of rows for symbols
-- which have 10-day sma crossing over 50-day sma as an example
--

DROP FUNCTION IF EXISTS public.find_sma_crossover(DATE);

CREATE OR REPLACE FUNCTION public.find_sma_crossover(
    IN start_date DATE DEFAULT current_date - 20
)
RETURNS TABLE (
    symbol TEXT,
    date DATE,
    short_sma_before NUMERIC(14, 4),
    longer_sma NUMERIC(14, 4),
    short_sma_after NUMERIC(14, 4)
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
        t1.short_sma_before,
        t1.longer_sma,
        t1.short_sma_after
    FROM
        (
        SELECT
            t0.symbol,
            t0.date,
            t0.sma_200 AS longer_sma,
            LAG(t0.sma_10, 1) OVER w AS short_sma_before,
            t0.sma_10 AS short_sma_after
        FROM
            new_stock_data.current_stats AS t0
        WHERE
            t0.date >= start_date
        WINDOW w AS (PARTITION BY t0.symbol ORDER BY t0.date ASC)
        ORDER BY
            t0.symbol, t0.date
        ) AS t1
        WHERE
            t1.short_sma_before < t1.longer_sma
            and t1.short_sma_after > t1.longer_sma ;
    RETURN;
END;
$BODY$

LANGUAGE 'plpgsql' STABLE
COST 100
ROWS 2000;

ALTER FUNCTION public.find_sma_crossover(DATE) OWNER TO data_updater_pg;
