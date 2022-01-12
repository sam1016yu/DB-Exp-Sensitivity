CREATE ALIAS ORDERSTATUS AS $$
import java.sql.*;
import java.util.Random;
import java.util.ArrayList;
@CODE
void orderStatus(Connection conn, int terminalDistrictLowerID, int terminalDistrictUpperID, 
    int numWarehouses, int w_id) throws SQLException
{

    String ordStatGetNewestOrdSQL =  "SELECT O_ID, O_CARRIER_ID, O_ENTRY_D " +
                    "  FROM oorder " +
                    " WHERE O_W_ID = ? " +
                    "   AND O_D_ID = ? " +
                    "   AND O_C_ID = ? " +
                    " ORDER BY O_ID DESC LIMIT 1";

    String ordStatGetOrderLinesSQL = "SELECT OL_I_ID, OL_SUPPLY_W_ID, OL_QUANTITY, OL_AMOUNT, OL_DELIVERY_D " +
                    "  FROM order_line " +
                    " WHERE OL_O_ID = ?" +
                    "   AND OL_D_ID = ?" +
                    "   AND OL_W_ID = ?";

    String payGetCustSQL = "SELECT C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, " +
                    "       C_CITY, C_STATE, C_ZIP, C_PHONE, C_CREDIT, C_CREDIT_LIM, " +
                    "       C_DISCOUNT, C_BALANCE, C_YTD_PAYMENT, C_PAYMENT_CNT, C_SINCE " +
                    "  FROM customer " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_ID = ?";

    String customerByNameSQL = "SELECT C_FIRST, C_MIDDLE, C_ID, C_STREET_1, C_STREET_2, C_CITY, " +
                    "       C_STATE, C_ZIP, C_PHONE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, " +
                    "       C_BALANCE, C_YTD_PAYMENT, C_PAYMENT_CNT, C_SINCE " +
                    "  FROM customer " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_LAST = ? " +
                    " ORDER BY C_FIRST";

    String[] nameTokens = {"BAR", "OUGHT", "ABLE", "PRI", "PRES", "ESE", "ANTI", "CALLY", "ATION", "EING"};

    try (PreparedStatement ordStatGetNewestOrd = conn.prepareStatement(ordStatGetNewestOrdSQL);
        PreparedStatement ordStatGetOrderLines = conn.prepareStatement(ordStatGetOrderLinesSQL)) {

        Random gen = new Random();
        int c_id = 0;
        int c_payment_cnt = 0;
        int c_delivery_cnt;
        Timestamp c_since;
        float c_discount;
        float c_credit_lim;
        float c_balance = 0;
        float c_ytd_payment = 0;
        String c_credit = null; 
        String c_last;
        String c_first;
        String c_street_1;
        String c_street_2;
        String c_city;
        String c_state;
        String c_zip;
        String c_phone;
        String c_middle;
        String c_data = null;

        
        int districtID = (int) (gen.nextDouble() * (terminalDistrictUpperID - terminalDistrictLowerID + 1) + terminalDistrictLowerID);
        boolean c_by_name;
        int y = (int) (gen.nextDouble() * (100 - 1 + 1) + 1);
        String customerLastName = null;
        int customerID = -1;

        if (y <= 60) {
            c_by_name = true;
            int num = ((((int) (gen.nextDouble() * (255 - 0 + 1) + 0) | (int) (gen.nextDouble() * (999 - 0 + 1) + 0)) + 223) % (999 - 0 + 1)) + 0;
            customerLastName = nameTokens[num / 100] + nameTokens[(num / 10) % 10]
                + nameTokens[num % 10];
        } else {
            c_by_name = false;
            customerID = ((((int) (gen.nextDouble() * (1023 - 0 + 1) + 0) | (int) (gen.nextDouble() * (3000 - 1 + 1) + 1)) + 259) % (3000 - 1 + 1)) + 1;
        }

        int o_id;
        int o_carrier_id;
        Timestamp o_entry_d;
        ArrayList<String> orderLines = new ArrayList<>();
        ArrayList<String> c_firsts = new ArrayList<>();
        ArrayList<String> c_middles = new ArrayList<>();
        ArrayList<Float> c_balances = new ArrayList<>();
        ArrayList<Integer> c_ids = new ArrayList<>();

        if (c_by_name) {
            try (PreparedStatement customerByNameS = conn.prepareStatement(customerByNameSQL)) {

                customerByNameS.setInt(1, w_id);
                customerByNameS.setInt(2, districtID);
                customerByNameS.setString(3, customerLastName);
                try (ResultSet rs = customerByNameS.executeQuery()) {

                    while (rs.next()) {
                        c_first = rs.getString("c_first");
                        c_middle = rs.getString("c_middle");
                        c_balance = rs.getFloat("c_balance");
                        c_id = rs.getInt("C_ID");
                        c_firsts.add(c_first);
                        c_middles.add(c_middle);
                        c_balances.add(c_balance);
                        c_ids.add(c_id);
                        //c_last = customerLastName;
                    }
                }
            }

            if (c_ids.size() == 0) {
                String msg = String.format("Failed to get CUSTOMER [C_W_ID=%d, C_D_ID=%d, C_LAST=%s]",
                    w_id, districtID, customerLastName);
                throw new RuntimeException(msg);
            }
            int index = c_ids.size() / 2;
            if (c_ids.size() % 2 == 0) {
                index -= 1;
            }
            c_first = c_firsts.get(index);
            c_middle = c_middles.get(index);
            c_balance = c_balances.get(index);
            c_id = c_ids.get(index);
            c_last = customerLastName;
        } else {
            try (PreparedStatement payGetCust = conn.prepareStatement(payGetCustSQL)) {

                payGetCust.setInt(1, w_id);
                payGetCust.setInt(2, districtID);
                payGetCust.setInt(3, customerID);

                try (ResultSet rs = payGetCust.executeQuery()) {
                    if (!rs.next()) {
                        throw new RuntimeException("C_ID=" + customerID + " C_D_ID=" + districtID + " C_W_ID=" + w_id + " not found!");
                    }
                    c_last = rs.getString("C_LAST");
                    c_first = rs.getString("c_first");
                    c_middle = rs.getString("c_middle");
                    c_balance = rs.getFloat("c_balance");
                    c_id = customerID;
                }
            }
        }

        // find the newest order for the customer
        // retrieve the carrier & order date for the most recent order.

        ordStatGetNewestOrd.setInt(1, w_id);
        ordStatGetNewestOrd.setInt(2, districtID);
        ordStatGetNewestOrd.setInt(3, c_id);

        try (ResultSet rs = ordStatGetNewestOrd.executeQuery()) {
            if (!rs.next()) {
                String msg = String.format("No order records for CUSTOMER [C_W_ID=%d, C_D_ID=%d, C_ID=%d]",
                            w_id, districtID, c_id);
                throw new RuntimeException(msg);
            }

            o_id = rs.getInt("O_ID");
            o_carrier_id = rs.getInt("O_CARRIER_ID");
            o_entry_d = rs.getTimestamp("O_ENTRY_D");
        }

        // retrieve the order lines for the most recent order
        ordStatGetOrderLines.setInt(1, o_id);
        ordStatGetOrderLines.setInt(2, districtID);
        ordStatGetOrderLines.setInt(3, w_id);
        try (ResultSet rs = ordStatGetOrderLines.executeQuery()) {
            String dS = "";
            while (rs.next()) {
                StringBuilder sb = new StringBuilder();
                sb.append("[");
                sb.append(rs.getLong("OL_SUPPLY_W_ID"));
                sb.append(" - ");
                sb.append(rs.getLong("OL_I_ID"));
                sb.append(" - ");
                sb.append(rs.getLong("OL_QUANTITY"));
                sb.append(" - ");
                dS = dS + rs.getDouble("OL_AMOUNT");
                sb.append(dS.length() > 6 ? dS.substring(0, 6) : dS);
                //sb.append(TPCCUtil.formattedDouble(rs.getDouble("OL_AMOUNT")));
                sb.append(" - ");
                if (rs.getTimestamp("OL_DELIVERY_D") != null) {
                    sb.append(rs.getTimestamp("OL_DELIVERY_D"));
                } else {
                    sb.append("99-99-9999");
                }
                sb.append("]");
                orderLines.add(sb.toString());
            }
        }


        if (orderLines.isEmpty()) {
            String msg = String.format("Order record had no order line items [C_W_ID=%d, C_D_ID=%d, C_ID=%d, O_ID=%d]",
                        w_id, districtID, c_id, o_id);
            System.out.println(msg);
        }

    }

}
$$;


