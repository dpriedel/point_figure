DROP TABLE IF EXISTS new_stock_data.current_stats;

CREATE TABLE new_stock_data.current_stats ( -- noqa: disable=L008
    symbol TEXT NOT NULL,
    date DATE NOT NULL,
    adjclose NUMERIC(12,4) DEFAULT '0',
    adjvolume BIGINT DEFAULT '1',
    pct_close_chg NUMERIC(14, 4) DEFAULT '0',
    pct_volume_chg NUMERIC(14, 4) DEFAULT '0',
    sma_10 NUMERIC(14, 4) DEFAULT '0',
    sma_50 NUMERIC(14, 4) DEFAULT '0',
    sma_200 NUMERIC(14, 4) DEFAULT '0',
    adjclose_std_100 NUMERIC(14, 4) DEFAULT '0',

    PRIMARY KEY (symbol, date)
);
ALTER TABLE new_stock_data.current_stats OWNER TO data_updater_pg;
