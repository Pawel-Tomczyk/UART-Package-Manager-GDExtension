extends RefCounted
class_name UartPacketCodec

# Runtime packet IDs - define any IDs you need without recompiling the extension.
const PKT_IMU_DATA: int = 0x20

# Example schema for:
# struct IMUData { double x; double y; double z; double mag; }
const IMU_SCHEMA: Array[Dictionary] = [
	{"name": "x", "type": "f64"},
	{"name": "y", "type": "f64"},
	{"name": "z", "type": "f64"},
	{"name": "mag", "type": "f64"},
]

# Supported field types: u8, i8, u16, i16, u32, i32, u64, i64, f32, f64.
# Schema format: [{"name": "field_name", "type": "f64"}, ...]
static func pack_by_schema(values: Dictionary, schema: Array[Dictionary], big_endian: bool = true) -> PackedByteArray:
	var sp := StreamPeerBuffer.new()
	sp.big_endian = big_endian

	for field in schema:
		var field_name: String = String(field.get("name", ""))
		var field_type: String = String(field.get("type", ""))

		if field_name.is_empty() or field_type.is_empty():
			push_error("Schema field must contain 'name' and 'type'.")
			return PackedByteArray()

		if not values.has(field_name):
			push_error("Missing field value: %s" % field_name)
			return PackedByteArray()

		_write_field(sp, field_type, values[field_name])

	return sp.data_array

static func unpack_by_schema(data: PackedByteArray, schema: Array[Dictionary], big_endian: bool = true) -> Dictionary:
	var sp := StreamPeerBuffer.new()
	sp.big_endian = big_endian
	sp.data_array = data
	sp.seek(0)

	var out: Dictionary = {}
	for field in schema:
		var field_name: String = String(field.get("name", ""))
		var field_type: String = String(field.get("type", ""))

		if field_name.is_empty() or field_type.is_empty():
			push_error("Schema field must contain 'name' and 'type'.")
			return {}

		out[field_name] = _read_field(sp, field_type)

	if sp.get_position() != sp.get_size():
		push_warning("Payload has trailing bytes: %d" % (sp.get_size() - sp.get_position()))

	return out

static func pack_imu_data(x: float, y: float, z: float, mag: float, big_endian: bool = true) -> PackedByteArray:
	return pack_by_schema(
		{"x": x, "y": y, "z": z, "mag": mag},
		IMU_SCHEMA,
		big_endian
	)

static func unpack_imu_data(data: PackedByteArray, big_endian: bool = true) -> Dictionary:
	return unpack_by_schema(data, IMU_SCHEMA, big_endian)

static func _write_field(sp: StreamPeerBuffer, field_type: String, value: Variant) -> void:
	match field_type:
		"u8":
			sp.put_u8(int(value))
		"i8":
			sp.put_8(int(value))
		"u16":
			sp.put_u16(int(value))
		"i16":
			sp.put_16(int(value))
		"u32":
			sp.put_u32(int(value))
		"i32":
			sp.put_32(int(value))
		"u64":
			sp.put_u64(int(value))
		"i64":
			sp.put_64(int(value))
		"f32":
			sp.put_float(float(value))
		"f64":
			sp.put_double(float(value))
		_:
			push_error("Unsupported field type: %s" % field_type)

static func _read_field(sp: StreamPeerBuffer, field_type: String) -> Variant:
	match field_type:
		"u8":
			return sp.get_u8()
		"i8":
			return sp.get_8()
		"u16":
			return sp.get_u16()
		"i16":
			return sp.get_16()
		"u32":
			return sp.get_u32()
		"i32":
			return sp.get_32()
		"u64":
			return sp.get_u64()
		"i64":
			return sp.get_64()
		"f32":
			return sp.get_float()
		"f64":
			return sp.get_double()
		_:
			push_error("Unsupported field type: %s" % field_type)
			return null
