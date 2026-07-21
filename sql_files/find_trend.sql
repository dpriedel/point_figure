SELECT
    symbol,
    date,
    adjclose
FROM
    (
        SELECT
            date,
            symbol,
            adjclose,
            LEAD(adjclose, 1) OVER w AS a,
            LEAD(adjclose, 2) OVER w AS b,
            LEAD(adjclose, 3) OVER w AS c,
            LEAD(adjclose, 4) OVER w AS d
        FROM
            new_stock_data.current_data
        WHERE
            date > '2023-08-01'
            AND adjclose > 10
            AND adjvolume > 100000
        WINDOW w AS (PARTITION BY symbol ORDER BY date ASC)
        ORDER BY
            symbol, date
    ) AS t1
WHERE
    adjclose < a AND a < b AND b < c AND c < d;
