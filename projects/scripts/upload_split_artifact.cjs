#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');
const { pipeline } = require('stream/promises');
const { DefaultArtifactClient } = require('@actions/artifact');

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
	const [
		archiveArg,
		artifactPrefix = 'eepp-dev-linux-x86_64',
		chunkSizeArg = '400M',
		retentionDaysArg = '5',
		maxPartsArg = '8',
	] = process.argv.slice(2);

	if (!archiveArg) {
		console.error(
			'Usage: upload_split_artifact.cjs <archive> [artifact-prefix] [chunk-size] [retention-days] [max-parts]'
		);
		process.exit(2);
	}

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

	console.log(
		`Uploading ${archive} (${stat.size} bytes) as ${partCount} artifact part(s) of at most ${chunkSizeArg}`
	);

	const artifact = new DefaultArtifactClient();
	const suffixWidth = Math.max(2, String(partCount - 1).length);

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
