-- this function will generate a series of rows for either the specific ticker symbol or all 
-- symbols from major US exchanges over the specified date range, if provided, or all available data.
-- with symbol, date, split_adj_close, split_adj_volume, percent change in close and volume, 10-day, 50-day and
-- 200-day simplemoving avg and running standard deviation (sample) for split_adj_close.
-- how to skip early rows when computing moving avg

-- select date,
-- (case when row_number() over (order by date) >= 21
--      then avg(col) over (order by date rows between 21 preceding and 1 preceding)
-- end) as mov_avg
-- from eurusd_ohlc

-- add an update_moving average function which just takes prior value, divide by 100, multiplies by
-- (n - 1), add new value, divide by n then multiple by 100

DROP FUNCTION IF EXISTS new_stock_data.init_daily_closing_stats(TEXT, DATE, DATE);

CREATE OR REPLACE FUNCTION new_stock_data.init_daily_closing_stats(
    IN all_or_symbol TEXT DEFAULT '*',
    IN start_date DATE DEFAULT '2017-01-01',
    IN end_date DATE DEFAULT current_date
)
RETURNS TABLE (
    symbol TEXT,
    date DATE,
    split_adj_close NUMERIC(14, 4),
    split_adj_volume BIGINT,
    pct_close_chg NUMERIC(14, 4),
    pct_volume_chg NUMERIC(14, 4),
    sma_10 NUMERIC(14, 4),
    sma_50 NUMERIC(14, 4),
    sma_200 NUMERIC(14, 4),
    split_adj_close_std_100 NUMERIC(14, 4)
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
            t1.split_adj_close,
            t1.split_adj_volume,
            (((t1.split_adj_close - LAG(t1.split_adj_close, 1) OVER w) / (LAG(t1.split_adj_close, 1) OVER w)) * 100.0)::NUMERIC(14, 4)
            AS pct_close_chg,
            (((t1.split_adj_volume - LAG(t1.split_adj_volume, 1) OVER w)::REAL / (LAG(t1.split_adj_volume, 1) OVER w)::REAL) * 100.0)::NUMERIC(14, 4)
            AS pct_volume_chg,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 9 THEN
                AVG(t1.split_adj_close) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 9 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END) ::NUMERIC(14, 4) AS sma_10,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 49 THEN
                AVG(t1.split_adj_close) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 49 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END) ::NUMERIC(14, 4) AS sma_50,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 199 THEN
                AVG(t1.split_adj_close) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 199 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END) ::NUMERIC(14, 4) AS sma_200,
            (CASE WHEN row_number() OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC) > 99 THEN
                STDDEV_SAMP(t1.split_adj_close) 
                OVER (PARTITION BY t1.symbol ORDER BY t1.date ASC ROWS BETWEEN 99 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS split_adj_close_std_100
        FROM
            new_stock_data.current_data AS t1
        INNER JOIN new_stock_data.names_and_symbols AS t2 ON t1.symbol = t2.symbol
        WHERE t1.split_adj_volume > 0 AND t1.split_adj_close > 0.0 AND (t1.date BETWEEN start_date AND end_date) AND t2.exchange IN ('BATS', 'NASDAQ', 'NYSE')
        WINDOW w AS (PARTITION BY t1.symbol ORDER BY t1.date ASC)
        ORDER BY t1.symbol, t1.date;
	    RETURN;
    ELSE
	    RETURN QUERY
        SELECT
            t3.symbol,
            t3.date,
            t3.split_adj_close,
            t3.split_adj_volume,
            (((t3.split_adj_close - LAG(t3.split_adj_close, 1) OVER w) / (LAG(t3.split_adj_close, 1) OVER w)) * 100.0)::NUMERIC(14, 4)
            AS pct_close_chg,
            (((t3.split_adj_volume - LAG(t3.split_adj_volume, 1) OVER w)::REAL / (LAG(t3.split_adj_volume, 1) OVER w)::REAL) * 100.0)::NUMERIC(14, 4)
            AS pct_volume_chg,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 9 THEN
                AVG(t3.split_adj_close) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 9 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS sma_10,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 49 THEN
                AVG(t3.split_adj_close) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 49 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS sma_50,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 199 THEN
                AVG(t3.split_adj_close) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 199 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS sma_200,
            (CASE WHEN row_number() OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC) > 99 THEN
                STDDEV_SAMP(t3.split_adj_close) 
                OVER (PARTITION BY t3.symbol ORDER BY t3.date ASC ROWS BETWEEN 99 PRECEDING AND CURRENT ROW)
            ELSE
                0
            END)::NUMERIC(14, 4) AS split_adj_close_std_100
        FROM
            new_stock_data.current_data AS t3
        WHERE t3.symbol = all_or_symbol AND t3.split_adj_volume > 0 AND t3.split_adj_close > 0.0 AND (t3.date BETWEEN start_date AND end_date)
        WINDOW w AS (PARTITION BY t3.symbol ORDER BY t3.date ASC)
        ORDER BY t3.symbol, t3.date;
	    RETURN;
    END IF;
END;
$BODY$

LANGUAGE 'plpgsql' STABLE
COST 100
ROWS 200000;

ALTER FUNCTION new_stock_data.init_daily_closing_stats(TEXT, DATE, DATE) OWNER TO data_updater_pg;
