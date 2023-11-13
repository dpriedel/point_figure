-- this function will generate a list of symbols from the
-- specified exchange which have split_adj_close prices >= the supplied value
-- checking from the given date.
--

DROP FUNCTION IF EXISTS new_stock_data.find_symbols_gte_min_split_adj_close(TEXT, NUMERIC, DATE);

CREATE OR REPLACE FUNCTION new_stock_data.find_symbols_gte_min_split_adj_close(
    IN xchng TEXT,
    IN min_close NUMERIC(14, 4) DEFAULT 5.0,
    IN start_date DATE DEFAULT current_date - interval '6 months'
)
RETURNS SETOF TEXT

AS
$BODY$

-- DECLARE
-- 	period INT := $4 - 1;

BEGIN
    RETURN QUERY
        SELECT
            t3.symbol
        FROM
            (
                SELECT
                    t1.symbol,
                    MIN(t1.split_adj_close) AS min_split_adj_close
                FROM
                    new_stock_data.current_data AS t1
                INNER JOIN new_stock_data.names_and_symbols AS t2
                    ON t1.symbol = t2.symbol
                WHERE
                    t2.exchange = xchng
                GROUP BY
                    t1.symbol
                ORDER BY
                    t1.symbol
            ) AS t3
        WHERE
            t3.min_split_adj_close >= min_close
        ORDER BY
            t3.symbol;
    RETURN;
END;
$BODY$

LANGUAGE 'plpgsql' STABLE
COST 100
ROWS 2000;

ALTER FUNCTION new_stock_data.find_symbols_gte_min_split_adj_close(TEXT, NUMERIC, DATE) OWNER TO data_updater_pg;
