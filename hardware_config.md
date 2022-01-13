# Hardware Configurations

All node alias is for [Cloudlab](https://www.cloudlab.us/) except [OSC](https://www.osc.edu/) Owens.

<table class="tg">
<thead>
  <tr>
    <th class="tg-uzvj"><span style="font-weight:bold">Name</span></th>
    <th class="tg-uzvj"><span style="font-weight:bold">Original Settings or Node Alias</span></th>
    <th class="tg-uzvj"><span style="font-weight:bold">CPU</span></th>
    <th class="tg-uzvj"><span style="font-weight:bold">RAM</span></th>
    <th class="tg-uzvj"><span style="font-weight:bold">Disk</span></th>
    <th class="tg-wa1i"><span style="font-weight:bold">NIC</span></th>
  </tr>
</thead>
<tbody>
  <tr>
    <td class="tg-9wq8" rowspan="2">Calvin</td>
    <td class="tg-9wq8">original (Amazon EC2 m3.2xlarge)</td>
    <td class="tg-9wq8">8vCPU</td>
    <td class="tg-9wq8">30 GB</td>
    <td class="tg-9wq8">2 x 80GB</td>
    <td class="tg-nrix">Unknown</td>
  </tr>
  <tr>
    <td class="tg-9wq8">d430</td>
    <td class="tg-9wq8">2x&nbsp;&nbsp;Intel E5-2630 v3 8-core CPUs at 2.40 GHz (Haswell w/ EM64T)</td>
    <td class="tg-9wq8">64GB</td>
    <td class="tg-9wq8">2 x 1TB HDDs</td>
    <td class="tg-nrix">Unknown</td>
  </tr>
  <tr>
    <td class="tg-9wq8" rowspan="2">Silo</td>
    <td class="tg-9wq8">original</td>
    <td class="tg-9wq8">4x Intel Xeon E7-4830 8-core CPU at 2.1 GHz</td>
    <td class="tg-9wq8">256 GB</td>
    <td class="tg-9wq8">3 x Fusion IO ioDrive 2,&nbsp;&nbsp;6 x&nbsp;&nbsp;7200RPM SATA HDD</td>
    <td class="tg-nrix">Unknown</td>
  </tr>
  <tr>
    <td class="tg-9wq8">c6320</td>
    <td class="tg-9wq8">2x&nbsp;&nbsp;Intel E5-2683 v3 14-core CPUs at 2.00 GHz (Haswell)</td>
    <td class="tg-9wq8">256GB ECC</td>
    <td class="tg-9wq8">2x&nbsp;&nbsp;1 TB 7.2K RPM 3G SATA HDDs</td>
    <td class="tg-nrix">Dual-port Intel 10Gbe NIC (X520), Qlogic QLE 7340 40 Gb/s Infiniband HCA (PCIe v3.0, 8 lanes)</td>
  </tr>
  <tr>
    <td class="tg-9wq8" rowspan="2">HERD</td>
    <td class="tg-9wq8">original</td>
    <td class="tg-9wq8">Intel Xeon E5-2450 CPUs</td>
    <td class="tg-9wq8">Unknown</td>
    <td class="tg-9wq8">Unknown</td>
    <td class="tg-nrix">ConnectX-3<br>MX354A (56 Gbps IB) via PCIe 3.0 x8</td>
  </tr>
  <tr>
    <td class="tg-9wq8">r320</td>
    <td class="tg-9wq8">1x Xeon E5-2450 processor (8 cores, 2.1Ghz)</td>
    <td class="tg-9wq8">16GB (4 x 2GB RDIMMs, 1.6Ghz)</td>
    <td class="tg-9wq8">4 x 500GB 7.2K SATA Drives (RAID5)</td>
    <td class="tg-nrix">1GbE Dual port embedded NIC (Broadcom), 1 x Mellanox MX354A Dual port FDR CX3 adapter w/1 x QSA adapter</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">MICA</td>
    <td class="tg-nrix">original</td>
    <td class="tg-nrix">2 x Intel Xeon E5-2680 (8-core @2.70 GHz)</td>
    <td class="tg-nrix">64 GiB</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">4 x 10GB Intel X520-T2</td>
  </tr>
  <tr>
    <td class="tg-nrix">c6220</td>
    <td class="tg-nrix">2 x Xeon E5-2650v2 processors (8 cores each, 2.6Ghz)</td>
    <td class="tg-nrix">64GB (8 x 8GB DDR-3 RDIMMs, 1.86Ghz)</td>
    <td class="tg-nrix">2 x 1TB SATA 3.5” 7.2K rpm hard drives</td>
    <td class="tg-nrix">4 x 1GbE embedded Ethernet Ports (Broadcom), 1 x Intel X520 PCIe Dual port 10Gb Ethernet NIC, 1 x Mellanox FDR CX3 Single port mezz card</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">DrTM</td>
    <td class="tg-nrix">original</td>
    <td class="tg-nrix">2 x 10-core RTM-enabled Intel Xeon processors</td>
    <td class="tg-nrix">64 GB</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">Mellanox ConnectX-3 56Gbps InfiniBand</td>
  </tr>
  <tr>
    <td class="tg-nrix">OSC Owens</td>
    <td class="tg-nrix">2 x Intel E5-2680 v4 (14-core, 2.40 GHz)</td>
    <td class="tg-nrix">128GB</td>
    <td class="tg-nrix">1 x 1TB HDD</td>
    <td class="tg-nrix"> 100 Gb/s Infiniband EDR</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">TAPIR</td>
    <td class="tg-nrix">original (Google Compute Engine)</td>
    <td class="tg-nrix">virtualized, single core 2.6 GHz Intel Xeon</td>
    <td class="tg-nrix">8GB</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">1 Gb</td>
  </tr>
  <tr>
    <td class="tg-nrix">c220g1</td>
    <td class="tg-nrix">2x&nbsp;&nbsp;Intel E5-2630 v3 8-core CPUs at 2.40 GHz (Haswell w/ EM64T)</td>
    <td class="tg-nrix">128GB ECC (8x 16 GB DDR4 1866 MHz dual rank RDIMMs)</td>
    <td class="tg-nrix">2x&nbsp;&nbsp;1.2 TB 10K RPM 6G SAS SFF HDDs, One Intel DC S3500 480 GB 6G SATA SSDs</td>
    <td class="tg-nrix">Dual-port Intel X520-DA2 10Gb NIC (PCIe v3.0, 8 lanes), Onboard Intel i350 1Gb</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">Janus</td>
    <td class="tg-nrix">original (Amazon EC2 m4.large)</td>
    <td class="tg-nrix">2vCPU</td>
    <td class="tg-nrix">8GB</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">Unknown</td>
  </tr>
  <tr>
    <td class="tg-nrix">rs630</td>
    <td class="tg-nrix">2 x Xeon E5-2660 v3 processors (10 cores each, 2.6Ghz or more)</td>
    <td class="tg-nrix">256GB (16 x 16GB DDR4 DIMMs)</td>
    <td class="tg-nrix">1 x 900GB 10K SAS Drive</td>
    <td class="tg-nrix">1GbE Quad port embedded NIC (Intel), 1 x Solarflare Dual port SFC9120 10G Ethernet NIC</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">Cicada</td>
    <td class="tg-nrix">original</td>
    <td class="tg-nrix">two Intel Xeon E5-2697 v3 CPUs (each with 14 cores and 35 MiB last level cache)</td>
    <td class="tg-nrix">128 GiB</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">Unknown</td>
  </tr>
  <tr>
    <td class="tg-nrix">c6320</td>
    <td class="tg-nrix">2x&nbsp;&nbsp;Intel E5-2683 v3 14-core CPUs at 2.00 GHz (Haswell)</td>
    <td class="tg-nrix">256GB ECC</td>
    <td class="tg-nrix">2x&nbsp;&nbsp;1 TB 7.2K RPM 3G SATA HDDs</td>
    <td class="tg-nrix">Dual-port Intel 10Gbe NIC (X520), Qlogic QLE 7340 40 Gb/s Infiniband HCA (PCIe v3.0, 8 lanes)</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">GAM</td>
    <td class="tg-nrix">original</td>
    <td class="tg-nrix">Intel Xeon E5-1620 V3 CPU (4-core at 3.5GHz)</td>
    <td class="tg-nrix">32 GB DDR3</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">40 Gbps Mellanox MCX353A-QCBT InfiniBand</td>
  </tr>
  <tr>
    <td class="tg-nrix">c6220</td>
    <td class="tg-nrix">2 x Xeon E5-2650v2 processors (8 cores each, 2.6Ghz)</td>
    <td class="tg-nrix">64GB (8 x 8GB DDR-3 RDIMMs, 1.86Ghz)</td>
    <td class="tg-nrix">2 x 1TB SATA 3.5” 7.2K rpm hard drives</td>
    <td class="tg-nrix">4 x 1GbE embedded Ethernet Ports (Broadcom), 1 x Intel X520 PCIe Dual port 10Gb Ethernet NIC, 1 x Mellanox FDR CX3 Single port mezz card</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">Star</td>
    <td class="tg-nrix">original (Amazon EC2 m5.4xlarge)</td>
    <td class="tg-nrix">16vCPU@2.5GHz</td>
    <td class="tg-nrix">64GB</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">Unknown</td>
  </tr>
  <tr>
    <td class="tg-nrix">c6220</td>
    <td class="tg-nrix">2 x Xeon E5-2650v2 processors (8 cores each, 2.6Ghz)</td>
    <td class="tg-nrix">64GB (8 x 8GB DDR-3 RDIMMs, 1.86Ghz)</td>
    <td class="tg-nrix">2 x 1TB SATA 3.5” 7.2K rpm hard drives</td>
    <td class="tg-nrix">4 x 1GbE embedded Ethernet Ports (Broadcom), 1 x Intel X520 PCIe Dual port 10Gb Ethernet NIC, 1 x Mellanox FDR CX3 Single port mezz card</td>
  </tr>
  <tr>
    <td class="tg-nrix" rowspan="2">Aria</td>
    <td class="tg-nrix">original (Amazon EC2 m5.4xlarge)</td>
    <td class="tg-nrix">16vCPU@2.5GHz</td>
    <td class="tg-nrix">64GB</td>
    <td class="tg-nrix">Unknown</td>
    <td class="tg-nrix">Unknown</td>
  </tr>
  <tr>
    <td class="tg-nrix">m510</td>
    <td class="tg-nrix">8-core Intel Xeon D-1548 at 2.0 GHz</td>
    <td class="tg-nrix">64GB ECC (4x 16 GB DDR4-2133 SO-DIMMs)</td>
    <td class="tg-nrix">256 GB NVMe flash storage</td>
    <td class="tg-nrix">Dual-port Mellanox ConnectX-3 10 GB NIC (PCIe v3.0, 8 lanes</td>
  </tr>
   <tr>
    <td class="tg-nrix">H2</td>
    <td class="tg-nrix">c220g5</td>
    <td class="tg-nrix">2 x Intel Xeon Silver 4114 10-core CPUs at 2.20 GHz</td>
    <td class="tg-nrix">192GB ECC DDR4-2666</td>
    <td class="tg-nrix">One 1 TB 7200 RPM 6G SAS HDs, One Intel DC S3500 480 GB 6G SATA SSD</td>
    <td class="tg-nrix">Dual-port Intel X520-DA2 10Gb NIC (PCIe v3.0, 8 lanes), Onboard Intel i350 1Gb</td>
  </tr>
  <tr>
    <td class="tg-nrix">MySQL</td>
    <td class="tg-nrix">m510</td>
    <td class="tg-nrix">8-core Intel Xeon D-1548 at 2.0 GHz</td>
    <td class="tg-nrix">64GB ECC (4x 16 GB DDR4-2133 SO-DIMMs)</td>
    <td class="tg-nrix">256 GB NVMe flash storage</td>
    <td class="tg-nrix">Dual-port Mellanox ConnectX-3 10 GB NIC (PCIe v3.0, 8 lanes</td>
  </tr>
</tbody>
</table>
