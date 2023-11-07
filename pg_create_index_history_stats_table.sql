DROP TABLE IF EXISTS new_stock_data.index_history_stats;

CREATE TABLE new_stock_data.index_history_stats ( -- noqa: disable=L008
    symbol TEXT NOT NULL,
    date DATE NOT NULL,
    close_p NUMERIC(12,4) DEFAULT '0',
    pct_close_chg NUMERIC(14, 4) DEFAULT '0',
    sma_10 NUMERIC(14, 4) DEFAULT '0',
    sma_50 NUMERIC(14, 4) DEFAULT '0',
    sma_200 NUMERIC(14, 4) DEFAULT '0',
    close_std_100 NUMERIC(14, 4) DEFAULT '0',

    PRIMARY KEY (symbol, date)
);
ALTER TABLE new_stock_data.index_history_stats OWNER TO data_updater_pg;
