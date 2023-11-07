-- this function will generate a series of rows for either the specific ticker symbol or all 
-- symbols from major US exchanges over the specified date range, if provided, or all available data.
-- with symbol, date, close_p, adjvolume, percent change in close and volume, 10-day, 50-day and
-- 200-day simplemoving avg and running standard deviation (sample) for close_p.
-- 
DROP FUNCTION IF EXISTS new_stock_data.init_index_history_stats(TEXT, DATE, DATE);

CREATE OR REPLACE FUNCTION new_stock_data.init_index_history_stats(
    IN all_or_symbol TEXT DEFAULT '*',
    IN start_date DATE DEFAULT '2017-01-01',
    IN end_date DATE DEFAULT current_date
)
RETURNS TABLE (
    symbol TEXT,
    date DATE,
    close_p NUMERIC(14, 4),
    pct_close_chg NUMERIC(14, 4),
    sma_10 NUMERIC(14, 4),
    sma_50 NUMERIC(14, 4),
    sma_200 NUMERIC(14, 4),
    close_std_100 NUMERIC(14, 4)
)
AS
$BODY$

-- DECLARE
-- 	period INT := $4 - 1;

BEGIN
    IF all_or_symbol = '*' THEN
	    RETURN QUERY
        SELECT
            t1.symbol,
            t1.date,
            t1.close_p,
            (((t1.close_p - LAG(t1.close_p, 1) OVER w) / (LAG(t1.close_p, 1) OVER w)) * 100.0)::NUMERIC(14, 4)
            AS pct_close_chg,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 9 THEN
                AVG(t1.close_p) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 9 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END) ::NUMERIC(14, 4) AS sma_10,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 49 THEN
                AVG(t1.close_p) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 49 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END) ::NUMERIC(14, 4) AS sma_50,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 199 THEN
                AVG(t1.close_p) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 199 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END) ::NUMERIC(14, 4) AS sma_200,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 99 THEN
                STDDEV_SAMP(t1.close_p) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 99 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS close_p_std_100
        FROM
            new_stock_data.index_history AS t1
        WHERE t1.date BETWEEN start_date AND end_date
        WINDOW w AS (PARTITION BY t1.symbol ORDER BY t1.date ASC)
        ORDER BY t1.symbol, t1.date;
	    RETURN;
    ELSE
	    RETURN QUERY
        SELECT
            t3.symbol,
            t3.date,
            t3.close_p,
            (((t3.close_p - LAG(t3.close_p, 1) OVER w) / (LAG(t3.close_p, 1) OVER w)) * 100.0)::NUMERIC(14, 4)
            AS pct_close_chg,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 9 THEN
                AVG(t3.close_p) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 9 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS sma_10,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 49 THEN
                AVG(t3.close_p) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 49 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS sma_50,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 199 THEN
                AVG(t3.close_p) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 199 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS sma_200,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 99 THEN
                STDDEV_SAMP(t3.close_p) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 99 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS close_p_std_100
        FROM
            new_stock_data.index_history AS t3
        WHERE t3.symbol = all_or_symbol AND (t3.date BETWEEN start_date AND end_date)
        WINDOW w AS (PARTITION BY t3.symbol ORDER BY t3.date ASC)
        ORDER BY t3.symbol, t3.date;
	    RETURN;
    END IF;
END;
$BODY$

LANGUAGE 'plpgsql' STABLE
COST 100
ROWS 200000;

ALTER FUNCTION new_stock_data.init_index_history_stats(TEXT, DATE, DATE) OWNER TO data_updater_pg;
