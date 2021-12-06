resource "yandex_dns_zone" "dns_zones" {
  for_each = var.dns_zones
  name        = each.value["name"]
  description = each.value["description"]
  zone        = each.value["zone"]
  public      = each.value["public"]
  labels      = each.value["labels"]
}

resource "yandex_dns_recordset" "dns_zones_rs" {
  for_each = var.dns_zones_rs
  zone_id = yandex_dns_zone.dns_zones[each.value["zone_id"]].id
  name    = each.value["name"]
  type    = each.value["type"]
  ttl     = each.value["ttl"]
  data    = each.value["data"]
}
