resource "yandex_vpc_network" "cluster" {
  name = var.name
}

resource "yandex_vpc_subnet" "cluster_subnets" {
  count = length(var.zones)

  name = "${var.name}-${var.zones[count.index]}"
  v4_cidr_blocks = [cidrsubnet(var.subnet, length(var.zones)+1, count.index)]
  zone = var.zones[count.index]
  network_id = yandex_vpc_network.cluster.id
#  route_table_id  = yandex_vpc_route_table.vpc_route_table[count.index].id
}

resource "yandex_vpc_route_table" "vpc_route_table" {
  count = length(var.zones)
  name = "${var.name}-out-via-bastion-${count.index}"
  folder_id = var.folder_id
  network_id = yandex_vpc_network.cluster.id

  static_route {
    destination_prefix = "0.0.0.0/0"
    next_hop_address   = cidrhost(cidrsubnet(var.subnet, length(var.zones)+1, 0),127)
  }
}
