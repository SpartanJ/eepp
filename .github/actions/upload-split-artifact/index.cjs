'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFileSync } = require('child_process');
const { pipeline } = require('stream/promises');

function input(name, fallback = '') {
	return process.env[`INPUT_${name.toUpperCase()}`] || fallback;
}

function parseSize(value) {
	const match = /^(\d+)([KMG]?)$/i.exec(value);
	if (!match)
		throw new Error(`Invalid size '${value}' (expected e.g. 400M)`);

	const multipliers = {
		'': 1,
		K: 1024,
		M: 1024 * 1024,
		G: 1024 * 1024 * 1024,
	};

	return Number(match[1]) * multipliers[match[2].toUpperCase()];
}

async function writeChunk(source, destination, start, end) {
	await pipeline(
		fs.createReadStream(source, { start, end }),
		fs.createWriteStream(destination)
	);
}

async function main() {
	const archiveArg = input('archive');
	const artifactPrefix = input('artifact_prefix', 'eepp-dev-linux-x86_64');
	const chunkSizeArg = input('chunk_size', '400M');
	const retentionDaysArg = input('retention_days', '5');
	const maxPartsArg = input('max_parts', '8');

	if (!archiveArg)
		throw new Error("Missing required input 'archive'");

	const archive = path.resolve(archiveArg);
	const chunkSize = parseSize(chunkSizeArg);
	const retentionDays = Number(retentionDaysArg);
	const maxParts = Number(maxPartsArg);
	const stat = await fs.promises.stat(archive);

	if (!stat.isFile())
		throw new Error(`Not a regular file: ${archive}`);
	if (!Number.isInteger(retentionDays) || retentionDays < 1)
		throw new Error(`Invalid retention days: ${retentionDaysArg}`);
	if (!Number.isInteger(maxParts) || maxParts < 1)
		throw new Error(`Invalid max parts: ${maxPartsArg}`);

	const partCount = Math.ceil(stat.size / chunkSize);
	if (partCount > maxParts) {
		throw new Error(
			`Archive requires ${partCount} artifacts at ${chunkSizeArg} per part, ` +
			`but the configured maximum is ${maxParts}. Increase the chunk size or artifact budget.`
		);
	}

	const installDir = path.join(
		process.env.RUNNER_TEMP || os.tmpdir(),
		'eepp-artifact-uploader'
	);
	fs.mkdirSync(installDir, { recursive: true });

	console.log('Installing @actions/artifact@5.0.3...');
	execFileSync(
		'npm',
		[
			'install',
			'--no-save',
			'--no-package-lock',
			'--prefix',
			installDir,
			'@actions/artifact@5.0.3',
		],
		{ stdio: 'inherit' }
	);

	const { DefaultArtifactClient } = require(
		path.join(installDir, 'node_modules', '@actions', 'artifact')
	);
	const artifact = new DefaultArtifactClient();
	const suffixWidth = Math.max(2, String(partCount - 1).length);

	console.log(
		`Uploading ${archive} (${stat.size} bytes) as ${partCount} artifact part(s) of at most ${chunkSizeArg}`
	);

	for (let index = 0; index < partCount; ++index) {
		const suffix = String(index).padStart(suffixWidth, '0');
		const partPath = `${archive}.part-${suffix}`;
		const start = index * chunkSize;
		const end = Math.min(stat.size, start + chunkSize) - 1;
		const artifactName = `${artifactPrefix}-part-${suffix}`;

		console.log(
			`Creating ${path.basename(partPath)} (${start}-${end}, ${end - start + 1} bytes)`
		);
		await writeChunk(archive, partPath, start, end);

		try {
			const { id, size } = await artifact.uploadArtifact(
				artifactName,
				[partPath],
				path.dirname(partPath),
				{
					retentionDays,
					compressionLevel: 0,
				}
			);
			console.log(`Uploaded ${artifactName}: id=${id}, size=${size}`);
		} finally {
			await fs.promises.rm(partPath, { force: true });
		}
	}
}

main().catch((error) => {
	console.error(error.stack || error.message || error);
	process.exit(1);
});
