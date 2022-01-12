CREATE ALIAS NEWORDER AS $$
import java.sql.*;
import java.util.Random;
@CODE
void newOrder(Connection conn, int terminalDistrictLowerID, int terminalDistrictUpperID, 
    int numWarehouses, int w_id) throws SQLException
{

    String stmtGetCustSQL = "SELECT C_DISCOUNT, C_LAST, C_CREDIT" +
                    "  FROM customer " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_ID = ?";

    String stmtGetWhseSQL = "SELECT W_TAX " +
                    "  FROM warehouse " +
                    " WHERE W_ID = ?";

    String stmtGetDistSQL = "SELECT D_NEXT_O_ID, D_TAX " +
                    "  FROM district " +
                    " WHERE D_W_ID = ? AND D_ID = ? FOR UPDATE";

    String stmtInsertNewOrderSQL = "INSERT INTO new_order " +
                    " (NO_O_ID, NO_D_ID, NO_W_ID) " +
                    " VALUES ( ?, ?, ?)";

    String stmtUpdateDistSQL = "UPDATE district " +
                    "   SET D_NEXT_O_ID = D_NEXT_O_ID + 1 " +
                    " WHERE D_W_ID = ? " +
                    "   AND D_ID = ?";

    String stmtInsertOOrderSQL = "INSERT INTO oorder " +
                    " (O_ID, O_D_ID, O_W_ID, O_C_ID, O_ENTRY_D, O_OL_CNT, O_ALL_LOCAL)" +
                    " VALUES (?, ?, ?, ?, ?, ?, ?)";

    String stmtGetItemSQL = "SELECT I_PRICE, I_NAME , I_DATA " +
                    "  FROM item " +
                    " WHERE I_ID = ?";

    String stmtGetStockSQL = "SELECT S_QUANTITY, S_DATA, S_DIST_01, S_DIST_02, S_DIST_03, S_DIST_04, S_DIST_05, " +
                    "       S_DIST_06, S_DIST_07, S_DIST_08, S_DIST_09, S_DIST_10" +
                    "  FROM stock " +
                    " WHERE S_I_ID = ? " +
                    "   AND S_W_ID = ? FOR UPDATE";

    String stmtUpdateStockSQL = "UPDATE stock " +
                    "   SET S_QUANTITY = ? , " +
                    "       S_YTD = S_YTD + ?, " +
                    "       S_ORDER_CNT = S_ORDER_CNT + 1, " +
                    "       S_REMOTE_CNT = S_REMOTE_CNT + ? " +
                    " WHERE S_I_ID = ? " +
                    "   AND S_W_ID = ?";

    String stmtInsertOrderLineSQL = "INSERT INTO order_line " +
                    " (OL_O_ID, OL_D_ID, OL_W_ID, OL_NUMBER, OL_I_ID, OL_SUPPLY_W_ID, OL_QUANTITY, OL_AMOUNT, OL_DIST_INFO) " +
                    " VALUES (?,?,?,?,?,?,?,?,?)";
    

    Random gen = new Random();

    int d_id = (int) (gen.nextDouble() * (terminalDistrictUpperID - terminalDistrictLowerID + 1) + terminalDistrictLowerID);
    int c_id = ((((int) (gen.nextDouble() * (1023 - 0 + 1) + 0) | (int) (gen.nextDouble() * (3000 - 1 + 1) + 1)) + 259) % (3000 - 1 + 1)) + 1;
    int o_ol_cnt = (int) (gen.nextDouble() * (15 - 5 + 1) + 5);

    int[] itemIDs = new int[o_ol_cnt];
    int[] supplierWarehouseIDs = new int[o_ol_cnt];
    int[] orderQuantities = new int[o_ol_cnt];
    int o_all_local = 1;

    for (int i = 0; i < o_ol_cnt; i++) {
        itemIDs[i] = ((((int) (gen.nextDouble() * (8191 - 0 + 1) + 0) | (int) (gen.nextDouble() * (100000 - 1 + 1) + 1)) + 7911) % (100000 - 1 + 1)) + 1;
        if ((int) (gen.nextDouble() * (100 - 1 + 1) + 1) > 1) {
            supplierWarehouseIDs[i] = w_id;
        } else {
                do {
                    supplierWarehouseIDs[i] = (int) (gen.nextDouble() * (numWarehouses - 1 + 1) + 1);
                }
                while (supplierWarehouseIDs[i] == w_id
                        && numWarehouses > 1);
                o_all_local = 0;
        }
        orderQuantities[i] = (int) (gen.nextDouble() * (10 - 1 + 1) + 1);
    }

    float i_price;
    int d_next_o_id;
    int o_id;
    int s_quantity;
    String s_dist_01;
    String s_dist_02;
    String s_dist_03;
    String s_dist_04;
    String s_dist_05;
    String s_dist_06;
    String s_dist_07;
    String s_dist_08;
    String s_dist_09;
    String s_dist_10;
    String ol_dist_info = null;

    int ol_supply_w_id;
    int ol_i_id;
    int ol_quantity;
    int s_remote_cnt_increment;
    float ol_amount;

    try (PreparedStatement stmtGetCust = conn.prepareStatement(stmtGetCustSQL);
        PreparedStatement  stmtGetWhse = conn.prepareStatement(stmtGetWhseSQL);
        PreparedStatement stmtGetDist = conn.prepareStatement(stmtGetDistSQL);
        PreparedStatement stmtInsertNewOrder = conn.prepareStatement(stmtInsertNewOrderSQL);
        PreparedStatement stmtUpdateDist = conn.prepareStatement(stmtUpdateDistSQL);
        PreparedStatement stmtInsertOOrder = conn.prepareStatement(stmtInsertOOrderSQL);
        PreparedStatement stmtGetItem = conn.prepareStatement(stmtGetItemSQL);
        PreparedStatement stmtGetStock = conn.prepareStatement(stmtGetStockSQL);
        PreparedStatement stmtUpdateStock = conn.prepareStatement(stmtUpdateStockSQL);
        PreparedStatement stmtInsertOrderLine = conn.prepareStatement(stmtInsertOrderLineSQL)) {

        try {
            stmtGetCust.setInt(1, w_id);
            stmtGetCust.setInt(2, d_id);
            stmtGetCust.setInt(3, c_id);
            try (ResultSet rs = stmtGetCust.executeQuery()) {
                if (!rs.next()) {
                    throw new RuntimeException("C_D_ID=" + d_id
                            + " C_ID=" + c_id + " not found!");
                }
            }

            stmtGetWhse.setInt(1, w_id);
            try (ResultSet rs = stmtGetWhse.executeQuery()) {
                if (!rs.next()) {
                    throw new RuntimeException("W_ID=" + w_id + " not found!");
                }
            }

            stmtGetDist.setInt(1, w_id);
            stmtGetDist.setInt(2, d_id);
            try (ResultSet rs = stmtGetDist.executeQuery()) {
                if (!rs.next()) {
                       throw new RuntimeException("D_ID=" + d_id + " D_W_ID=" + w_id
                               + " not found!");
                }
                d_next_o_id = rs.getInt("D_NEXT_O_ID");
            }

            stmtUpdateDist.setInt(1, w_id);
            stmtUpdateDist.setInt(2, d_id);
            int result = stmtUpdateDist.executeUpdate();
            if (result == 0) {
                throw new RuntimeException(
                        "Error!! Cannot update next_order_id on district for D_ID="
                                + d_id + " D_W_ID=" + w_id);
            }

            o_id = d_next_o_id;

            stmtInsertOOrder.setInt(1, o_id);
            stmtInsertOOrder.setInt(2, d_id);
            stmtInsertOOrder.setInt(3, w_id);
            stmtInsertOOrder.setInt(4, c_id);
            stmtInsertOOrder.setTimestamp(5, new Timestamp(System.currentTimeMillis()));
            stmtInsertOOrder.setInt(6, o_ol_cnt);
            stmtInsertOOrder.setInt(7, o_all_local);
            stmtInsertOOrder.executeUpdate();

            stmtInsertNewOrder.setInt(1, o_id);
            stmtInsertNewOrder.setInt(2, d_id);
            stmtInsertNewOrder.setInt(3, w_id);
            stmtInsertNewOrder.executeUpdate();

            for (int ol_number = 1; ol_number <= o_ol_cnt; ol_number++) {
                   ol_supply_w_id = supplierWarehouseIDs[ol_number - 1];
                   ol_i_id = itemIDs[ol_number - 1];
                   ol_quantity = orderQuantities[ol_number - 1];
                   stmtGetItem.setInt(1, ol_i_id);
                   try (ResultSet rs = stmtGetItem.executeQuery()) {
                        if (!rs.next()) {
                            throw new RuntimeException("item not found!");
                        }
                        i_price = rs.getFloat("I_PRICE");
                   }

                   stmtGetStock.setInt(1, ol_i_id);
                   stmtGetStock.setInt(2, ol_supply_w_id);
                   try (ResultSet rs = stmtGetStock.executeQuery()) {
                       if (!rs.next()) {
                           throw new RuntimeException("I_ID=" + ol_i_id
                                   + " not found!");
                       }
                       s_quantity = rs.getInt("S_QUANTITY");
                       s_dist_01 = rs.getString("S_DIST_01");
                       s_dist_02 = rs.getString("S_DIST_02");
                       s_dist_03 = rs.getString("S_DIST_03");
                       s_dist_04 = rs.getString("S_DIST_04");
                       s_dist_05 = rs.getString("S_DIST_05");
                       s_dist_06 = rs.getString("S_DIST_06");
                       s_dist_07 = rs.getString("S_DIST_07");
                       s_dist_08 = rs.getString("S_DIST_08");
                       s_dist_09 = rs.getString("S_DIST_09");
                       s_dist_10 = rs.getString("S_DIST_10");
                   }

                   if (s_quantity - ol_quantity >= 10) {
                       s_quantity -= ol_quantity;
                   } else {
                       s_quantity += -ol_quantity + 91;
                   }

                   if (ol_supply_w_id == w_id) {
                       s_remote_cnt_increment = 0;
                   } else {
                       s_remote_cnt_increment = 1;
                   }

                   stmtUpdateStock.setInt(1, s_quantity);
                   stmtUpdateStock.setInt(2, ol_quantity);
                   stmtUpdateStock.setInt(3, s_remote_cnt_increment);
                   stmtUpdateStock.setInt(4, ol_i_id);
                   stmtUpdateStock.setInt(5, ol_supply_w_id);
                   stmtUpdateStock.addBatch();

                   ol_amount = ol_quantity * i_price;

                   switch (d_id) {
                       case 1:
                           ol_dist_info = s_dist_01;
                           break;
                       case 2:
                           ol_dist_info = s_dist_02;
                           break;
                       case 3:
                           ol_dist_info = s_dist_03;
                           break;
                       case 4:
                           ol_dist_info = s_dist_04;
                           break;
                       case 5:
                           ol_dist_info = s_dist_05;
                           break;
                       case 6:
                           ol_dist_info = s_dist_06;
                           break;
                       case 7:
                           ol_dist_info = s_dist_07;
                           break;
                       case 8:
                           ol_dist_info = s_dist_08;
                           break;
                       case 9:
                           ol_dist_info = s_dist_09;
                           break;
                       case 10:
                           ol_dist_info = s_dist_10;
                           break;
                   }

                   stmtInsertOrderLine.setInt(1, o_id);
                   stmtInsertOrderLine.setInt(2, d_id);
                   stmtInsertOrderLine.setInt(3, w_id);
                   stmtInsertOrderLine.setInt(4, ol_number);
                   stmtInsertOrderLine.setInt(5, ol_i_id);
                   stmtInsertOrderLine.setInt(6, ol_supply_w_id);
                   stmtInsertOrderLine.setInt(7, ol_quantity);
                   stmtInsertOrderLine.setDouble(8, ol_amount);
                   stmtInsertOrderLine.setString(9, ol_dist_info);
                   stmtInsertOrderLine.addBatch();

            }

            stmtInsertOrderLine.executeBatch();
            stmtUpdateStock.executeBatch();

        } finally {
               if (stmtInsertOrderLine != null) {
                   stmtInsertOrderLine.clearBatch();
               }
               if (stmtUpdateStock != null) {
                   stmtUpdateStock.clearBatch();
               }
        }
    }

}
$$;