uniform sampler2D textureUnit0;
uniform vec2 textureRes;
uniform int dir;
const float radius = 16.;

float SCurve (float x) {
	x = x * 2.0 - 1.0;
	return -x * abs(x) * 0.5 + x + 0.5;
}

vec4 BlurH(sampler2D source, vec2 size, vec2 uv) {
	if (radius >= 1.0) {
		vec4 A = vec4(0.0);
		vec4 C = vec4(0.0);
		float width = 1.0 / size.x;
		float divisor = 0.0;
		float weight = 0.0;
		float radiusMultiplier = 1.0 / radius;

		for (float x = -radius; x <= radius; x++) {
			A = texture2D(source, uv + vec2(x * width, 0.0));
			weight = SCurve(1.0 - (abs(x) * radiusMultiplier));
			C += A * weight;
			divisor += weight;
		}

		return vec4(C.r / divisor, C.g / divisor, C.b / divisor, 1.0);
	}

	return texture2D(source, uv);
}

vec4 BlurV(sampler2D source, vec2 size, vec2 uv) {
	if (radius >= 1.0) {
		vec4 A = vec4(0.0);
		vec4 C = vec4(0.0);
		float height = 1.0 / size.y;
		float divisor = 0.0;
		float weight = 0.0;
		float radiusMultiplier = 1.0 / radius;

		for (float y = -radius; y <= radius; y++) {
			A = texture2D(source, uv + vec2(0.0, y * height));
			weight = SCurve(1.0 - (abs(y) * radiusMultiplier));
			C += A * weight;
			divisor += weight;
		}

		return vec4(C.r / divisor, C.g / divisor, C.b / divisor, 1.0);
	}

	return texture2D(source, uv);
}

void main()
{
	gl_FragColor = dir != 0 ?
					BlurV(textureUnit0, textureRes.xy, gl_TexCoord[0].xy) :
					BlurH(textureUnit0, textureRes.xy, gl_TexCoord[0].xy);
}
