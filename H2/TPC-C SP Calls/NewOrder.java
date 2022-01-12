/*
 * Copyright 2020 by OLTPBenchmark Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

package com.oltpbenchmark.benchmarks.tpcc.procedures;

import com.oltpbenchmark.api.SQLStmt;
import com.oltpbenchmark.benchmarks.tpcc.TPCCConfig;
import com.oltpbenchmark.benchmarks.tpcc.TPCCConstants;
import com.oltpbenchmark.benchmarks.tpcc.TPCCUtil;
import com.oltpbenchmark.benchmarks.tpcc.TPCCWorker;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.sql.*;
import java.util.Random;
import java.util.Arrays;

public class NewOrder extends TPCCProcedure {

    private static final Logger LOG = LoggerFactory.getLogger(NewOrder.class);

    public void run(Connection conn, Random gen, int terminalWarehouseID, int numWarehouses, int terminalDistrictLowerID, int terminalDistrictUpperID, TPCCWorker w) throws SQLException {

        newOrderTransaction(terminalDistrictLowerID, terminalDistrictUpperID, numWarehouses, terminalWarehouseID, conn);

    }


    private void newOrderTransaction(int terminalDistrictLowerID, int terminalDistrictUpperID, int numWarehouses, int w_id, Connection conn)
            throws SQLException {

            Statement st = conn.createStatement();
            String spcall = "call NEWORDER(" + terminalDistrictLowerID + ", " + terminalDistrictUpperID + ", " + numWarehouses + ", " + w_id + ");";
               
            st.executeQuery(spcall);
    }

}
