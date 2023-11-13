-- this table uses the split_adj_close and split_adj_volume fields because
-- those fields will have either the actual split-adjusted values or a
-- copy of the unadjusted close and volume fieds.  In other words,
-- these fields will always have something useful in them.

DROP TABLE IF EXISTS new_stock_data.current_stats;

CREATE TABLE new_stock_data.current_stats ( -- noqa: disable=L008
    symbol TEXT NOT NULL,
    date DATE NOT NULL,
    split_adj_close NUMERIC(12,4) DEFAULT '0',
    split_adj_volume BIGINT DEFAULT '1',
    pct_close_chg NUMERIC(14, 4) DEFAULT '0',
    pct_volume_chg NUMERIC(14, 4) DEFAULT '0',
    sma_10 NUMERIC(14, 4) DEFAULT '0',
    sma_50 NUMERIC(14, 4) DEFAULT '0',
    sma_200 NUMERIC(14, 4) DEFAULT '0',
    split_adj_close_std_100 NUMERIC(14, 4) DEFAULT '0',

    PRIMARY KEY (symbol, date)
);
ALTER TABLE new_stock_data.current_stats OWNER TO data_updater_pg;
